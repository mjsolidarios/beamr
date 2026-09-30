#include "recorder.h"

#include <QDir>
#include <QFileInfo>

#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mathematics.h>
}

#include <beamr/log.h>
#include <beamr/protocol.h>

namespace protocol = beamr::protocol;

namespace {

constexpr AVRational kMicroseconds{1, 1'000'000};

QString averror(int code)
{
    char text[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(code, text, sizeof text);
    return QString::fromUtf8(text);
}

// The SPS and PPS NAL units from an Annex B keyframe, still in Annex B,
// which is how the MP4 muxer wants its H.264 extradata.
QByteArray parameterSets(const QByteArray &annexB)
{
    QByteArray sets;
    const auto *data = reinterpret_cast<const uchar *>(annexB.constData());
    const qsizetype size = annexB.size();
    qsizetype i = 0;
    auto nextStart = [&](qsizetype from) {
        for (qsizetype j = from; j + 3 <= size; ++j) {
            if (data[j] == 0 && data[j + 1] == 0 && data[j + 2] == 1)
                return j;
        }
        return size;
    };
    i = nextStart(0);
    while (i < size) {
        const qsizetype nal = i + 3;
        qsizetype end = nextStart(nal);
        // A 4-byte start code's leading zero belongs to the next one.
        qsizetype trimmed = end;
        if (trimmed < size && trimmed > nal && data[trimmed - 1] == 0)
            --trimmed;
        if (nal < size) {
            const int type = data[nal] & 0x1f;
            if (type == 7 || type == 8) {
                sets.append("\x00\x00\x00\x01", 4);
                sets.append(reinterpret_cast<const char *>(data + nal), trimmed - nal);
            }
        }
        i = end;
    }
    return sets;
}

// Opus in MP4 needs the OpusHead the phone doesn't send; the format is fixed.
QByteArray opusHead()
{
    QByteArray head("OpusHead", 8);
    head.append(char(1));                         // version
    head.append(char(protocol::kAudioChannels));  // channels
    const quint16 preSkip = 312;                  // libopus's default encoder delay
    head.append(char(preSkip & 0xff)).append(char(preSkip >> 8));
    const quint32 rate = protocol::kAudioSampleRate;
    for (int shift = 0; shift < 32; shift += 8)
        head.append(char((rate >> shift) & 0xff));
    head.append(char(0)).append(char(0));         // output gain
    head.append(char(0));                         // mapping family: mono/stereo
    return head;
}

void setExtradata(AVCodecParameters *par, const QByteArray &data)
{
    par->extradata = static_cast<uint8_t *>(av_mallocz(size_t(data.size()) + AV_INPUT_BUFFER_PADDING_SIZE));
    std::memcpy(par->extradata, data.constData(), size_t(data.size()));
    par->extradata_size = int(data.size());
}

} // namespace

Recorder::Recorder(QObject *parent)
    : QObject(parent)
{
}

Recorder::~Recorder()
{
    stop();
}

void Recorder::start(const QString &path, QSize videoSize, bool withAudio)
{
    stop();
    m_path = path;
    m_videoSize = videoSize;
    m_withAudio = withAudio;
    m_active = true;
    m_paused = false;
    m_needKey = true;
    m_originUs = -1;
    m_pausedAtUs = -1;
    m_lastVideoTicks = -1;
    m_lastAudioTicks = -1;
}

void Recorder::setPaused(bool paused)
{
    if (!m_active || paused == m_paused)
        return;
    m_paused = paused;
    if (paused) {
        m_pausedAtUs = m_lastInputUs;
    } else {
        // Resume at a keyframe; the gap is removed when it arrives.
        m_needKey = true;
    }
}

bool Recorder::open(const QByteArray &keyframe)
{
    const QByteArray sets = parameterSets(keyframe);
    if (sets.isEmpty())
        return false; // Not a keyframe with its headers; wait for the next.

    QDir().mkpath(QFileInfo(m_path).absolutePath());
    const QByteArray path = m_path.toUtf8();
    int error = avformat_alloc_output_context2(&m_format, nullptr, "mp4", path.constData());
    if (error < 0 || !m_format) {
        fail(tr("can't create the file: %1").arg(averror(error)));
        return false;
    }

    m_videoStream = avformat_new_stream(m_format, nullptr);
    AVCodecParameters *video = m_videoStream->codecpar;
    video->codec_type = AVMEDIA_TYPE_VIDEO;
    video->codec_id = AV_CODEC_ID_H264;
    video->width = m_videoSize.width();
    video->height = m_videoSize.height();
    setExtradata(video, sets);
    m_videoStream->time_base = {1, 90'000};

    if (m_withAudio) {
        m_audioStream = avformat_new_stream(m_format, nullptr);
        AVCodecParameters *audio = m_audioStream->codecpar;
        audio->codec_type = AVMEDIA_TYPE_AUDIO;
        audio->codec_id = AV_CODEC_ID_OPUS;
        audio->sample_rate = protocol::kAudioSampleRate;
        av_channel_layout_default(&audio->ch_layout, protocol::kAudioChannels);
        audio->initial_padding = 312;
        setExtradata(audio, opusHead());
        m_audioStream->time_base = {1, protocol::kAudioSampleRate};
    }

    if ((error = avio_open(&m_format->pb, path.constData(), AVIO_FLAG_WRITE)) < 0) {
        fail(tr("can't write %1: %2").arg(QDir::toNativeSeparators(m_path), averror(error)));
        return false;
    }
    // Writes the index up front on stop, so the file opens quickly and
    // survives being copied before it's played.
    AVDictionary *options = nullptr;
    av_dict_set(&options, "movflags", "+faststart", 0);
    error = avformat_write_header(m_format, &options);
    av_dict_free(&options);
    if (error < 0) {
        fail(tr("can't start the file: %1").arg(averror(error)));
        return false;
    }
    qCInfo(lcCodec) << "recording to" << m_path;
    return true;
}

void Recorder::video(const QByteArray &packet, quint8 flags, qint64 ptsUs)
{
    m_lastInputUs = ptsUs;
    if (!m_active || m_paused)
        return;
    const bool key = flags & protocol::kFrameKey;
    if (flags & protocol::kFrameConfig)
        return; // SPS/PPS alone; keyframes repeat them.
    if (m_needKey) {
        if (!key)
            return;
        if (!m_format && !open(packet))
            return;
        if (m_originUs < 0) {
            m_originUs = ptsUs;
        } else if (m_pausedAtUs >= 0) {
            // Close the gap: continue one frame after where the pause began.
            m_originUs += (ptsUs - m_pausedAtUs);
            m_pausedAtUs = -1;
        }
        m_needKey = false;
    }
    write(m_videoStream, packet, ptsUs, key);
}

void Recorder::audio(const QByteArray &packet, qint64 ptsUs)
{
    if (!m_active || m_paused || m_needKey || !m_audioStream || !m_format)
        return;
    write(m_audioStream, packet, ptsUs, true);
}

void Recorder::write(AVStream *stream, const QByteArray &data, qint64 ptsUs, bool key)
{
    // In the stream's own ticks (90 kHz video, 48 kHz sound): each packet
    // must come strictly after the last, and none before the file's start.
    qint64 &last = stream == m_videoStream ? m_lastVideoTicks : m_lastAudioTicks;
    const qint64 fileUs = std::max<qint64>(0, ptsUs - m_originUs);
    qint64 ticks = av_rescale_q(fileUs, kMicroseconds, stream->time_base);
    if (ticks <= last)
        ticks = last + 1;
    last = ticks;

    AVPacket *packet = av_packet_alloc();
    if (av_new_packet(packet, int(data.size())) < 0) {
        av_packet_free(&packet);
        return;
    }
    std::memcpy(packet->data, data.constData(), size_t(data.size()));
    packet->stream_index = stream->index;
    packet->pts = packet->dts = ticks;
    if (key)
        packet->flags |= AV_PKT_FLAG_KEY;
    const int error = av_interleaved_write_frame(m_format, packet);
    av_packet_free(&packet);
    if (error < 0)
        fail(tr("writing failed: %1").arg(averror(error)));
}

void Recorder::stop()
{
    if (!m_active)
        return;
    const bool wrote = m_format && m_format->pb;
    close();
    m_active = false;
    if (wrote)
        emit finished(m_path, {});
    else
        emit finished({}, tr("nothing was recorded before it stopped"));
}

void Recorder::fail(const QString &error)
{
    qCWarning(lcCodec) << "recording failed:" << error;
    close();
    m_active = false;
    emit finished(m_path, error);
}

void Recorder::close()
{
    if (m_format) {
        if (m_format->pb) {
            av_write_trailer(m_format);
            avio_closep(&m_format->pb);
        }
        avformat_free_context(m_format);
    }
    m_format = nullptr;
    m_videoStream = nullptr;
    m_audioStream = nullptr;
}
