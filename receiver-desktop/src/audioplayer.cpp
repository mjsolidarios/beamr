#include "audioplayer.h"

#include <QAudioFormat>
#include <QAudioSink>
#include <QMediaDevices>

#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
}

#include <beamr/log.h>
#include <beamr/protocol.h>

namespace protocol = beamr::protocol;

namespace {

// Playback buffer: jitter room without audible lag.
constexpr int kBufferMs = 100;

QAudioFormat outputFormat()
{
    QAudioFormat format;
    format.setSampleRate(protocol::kAudioSampleRate);
    format.setChannelCount(protocol::kAudioChannels);
    format.setSampleFormat(QAudioFormat::Float);
    return format;
}

} // namespace

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject(parent)
{
}

AudioPlayer::~AudioPlayer()
{
    closeOutput();
    closeDecoder();
}

bool AudioPlayer::openDecoder()
{
    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_OPUS);
    if (!codec) {
        qCCritical(lcCodec) << "this FFmpeg build has no Opus decoder; phones will be silent";
        return false;
    }
    m_context = avcodec_alloc_context3(codec);
    m_packet = av_packet_alloc();
    m_frame = av_frame_alloc();
    if (!m_context || !m_packet || !m_frame)
        return false;

    // The phone sends raw Opus packets without an OpusHead; the format is fixed.
    m_context->sample_rate = protocol::kAudioSampleRate;
    av_channel_layout_default(&m_context->ch_layout, protocol::kAudioChannels);
    // Float out, so the samples go straight to the sink after interleaving.
    m_context->request_sample_fmt = AV_SAMPLE_FMT_FLT;

    if (avcodec_open2(m_context, codec, nullptr) < 0) {
        qCCritical(lcCodec) << "can't open the Opus decoder";
        return false;
    }
    qCInfo(lcCodec) << "Opus decoder ready";
    return true;
}

void AudioPlayer::closeDecoder()
{
    avcodec_free_context(&m_context);
    av_packet_free(&m_packet);
    av_frame_free(&m_frame);
}

bool AudioPlayer::openOutput()
{
    // The chosen output if it's still there, else the system's default.
    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (!m_deviceId.isEmpty()) {
        const QList<QAudioDevice> outputs = QMediaDevices::audioOutputs();
        for (const QAudioDevice &output : outputs) {
            if (output.id() == m_deviceId) {
                device = output;
                break;
            }
        }
    }
    const QAudioFormat format = outputFormat();
    if (device.isNull() || !device.isFormatSupported(format)) {
        qCWarning(lcCodec) << "no audio output for 48 kHz stereo float; phones will be silent";
        return false;
    }
    m_sink = std::make_unique<QAudioSink>(device, format);
    m_sink->setBufferSize(format.bytesForDuration(kBufferMs * 1000));
    m_sink->setVolume(m_volume);
    m_output = m_sink->start();
    return m_output != nullptr;
}

void AudioPlayer::setDevice(const QByteArray &id)
{
    if (id == m_deviceId)
        return;
    m_deviceId = id;
    // Reopened on the next packet, on the new device.
    closeOutput();
    m_failed = false;
}

void AudioPlayer::closeOutput()
{
    if (m_sink)
        m_sink->stop();
    m_output = nullptr;
    m_sink.reset();
}

void AudioPlayer::setVolume(qreal volume)
{
    m_volume = volume;
    if (m_sink)
        m_sink->setVolume(volume);
}

void AudioPlayer::reset()
{
    closeOutput();
    closeDecoder();
    m_failed = false;
}

void AudioPlayer::decode(const QByteArray &packet)
{
    if (m_failed)
        return;
    if (!m_context && !openDecoder()) {
        m_failed = true;
        closeDecoder();
        return;
    }
    if (!m_output && !openOutput()) {
        m_failed = true;
        closeOutput();
        return;
    }

    if (av_new_packet(m_packet, int(packet.size())) < 0)
        return;
    std::memcpy(m_packet->data, packet.constData(), size_t(packet.size()));
    int error = avcodec_send_packet(m_context, m_packet);
    av_packet_unref(m_packet);
    if (error < 0 && error != AVERROR(EAGAIN))
        return; // A lost packet is a click; carry on.

    while ((error = avcodec_receive_frame(m_context, m_frame)) >= 0) {
        const int channels = m_frame->ch_layout.nb_channels;
        const int samples = m_frame->nb_samples;
        const auto format = AVSampleFormat(m_frame->format);
        m_pcm.resize(qsizetype(samples) * protocol::kAudioChannels * sizeof(float));
        auto *out = reinterpret_cast<float *>(m_pcm.data());

        // Interleave, duplicating mono or dropping extra channels.
        for (int i = 0; i < samples; ++i) {
            for (int c = 0; c < protocol::kAudioChannels; ++c) {
                const int source = std::min(c, channels - 1);
                float value = 0;
                if (format == AV_SAMPLE_FMT_FLTP)
                    value = reinterpret_cast<const float *>(m_frame->data[source])[i];
                else if (format == AV_SAMPLE_FMT_FLT)
                    value = reinterpret_cast<const float *>(m_frame->data[0])[i * channels + source];
                else if (format == AV_SAMPLE_FMT_S16)
                    value = reinterpret_cast<const qint16 *>(m_frame->data[0])[i * channels + source] / 32768.0f;
                *out++ = value;
            }
        }
        av_frame_unref(m_frame);

        // Whatever doesn't fit in the buffer would only play late; drop it.
        const qsizetype room = m_sink->bytesFree();
        const qsizetype bytes = std::min(room, m_pcm.size());
        if (bytes > 0)
            m_output->write(m_pcm.constData(), bytes - bytes % (protocol::kAudioChannels * sizeof(float)));
    }
}
