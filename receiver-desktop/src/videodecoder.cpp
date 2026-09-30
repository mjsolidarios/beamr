#include "videodecoder.h"

#include <QVideoFrameFormat>

#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
}

#include <beamr/log.h>

namespace {

QString averror(int code)
{
    char text[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(code, text, sizeof text);
    return QString::fromUtf8(text);
}

} // namespace

VideoDecoder::VideoDecoder(QObject *parent)
    : QObject(parent)
{
}

VideoDecoder::~VideoDecoder()
{
    close();
}

bool VideoDecoder::open()
{
    const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        qCCritical(lcCodec) << "this FFmpeg build has no H.264 decoder";
        return false;
    }

    m_context = avcodec_alloc_context3(codec);
    m_packet = av_packet_alloc();
    m_frame = av_frame_alloc();
    if (!m_context || !m_packet || !m_frame)
        return false;

    // Show every frame the moment it's decoded. Frame threading would buffer
    // one frame per thread; slice threading doesn't add latency.
    m_context->flags |= AV_CODEC_FLAG_LOW_DELAY;
    m_context->thread_type = FF_THREAD_SLICE;
    m_context->thread_count = 0;

    if (const int error = avcodec_open2(m_context, codec, nullptr); error < 0) {
        qCCritical(lcCodec) << "can't open the H.264 decoder:" << averror(error);
        return false;
    }
    qCInfo(lcCodec) << "H.264 decoder ready";
    return true;
}

void VideoDecoder::close()
{
    avcodec_free_context(&m_context);
    av_packet_free(&m_packet);
    av_frame_free(&m_frame);
}

void VideoDecoder::reset()
{
    close();
    m_failed = false;
    m_lastLoggedFormat = -1;
}

void VideoDecoder::decode(const QByteArray &packet, int generation)
{
    if (m_failed)
        return;
    if (!m_context && !open()) {
        m_failed = true;
        close();
        return;
    }

    // av_new_packet() adds the zeroed padding the parser reads past the end.
    if (av_new_packet(m_packet, int(packet.size())) < 0)
        return;
    std::memcpy(m_packet->data, packet.constData(), size_t(packet.size()));

    int error = avcodec_send_packet(m_context, m_packet);
    av_packet_unref(m_packet);
    // A damaged packet only costs a frame; the next keyframe heals it.
    if (error < 0 && error != AVERROR(EAGAIN)) {
        qCDebug(lcCodec) << "decoder rejected a packet:" << averror(error);
        return;
    }

    while ((error = avcodec_receive_frame(m_context, m_frame)) >= 0) {
        QVideoFrame frame = toVideoFrame(m_frame);
        av_frame_unref(m_frame);
        if (frame.isValid())
            emit frameDecoded(frame, generation);
    }
}

QVideoFrame VideoDecoder::toVideoFrame(const AVFrame *source) const
{
    const auto format = AVPixelFormat(source->format);
    if (format != AV_PIX_FMT_YUV420P && format != AV_PIX_FMT_YUVJ420P) {
        if (m_lastLoggedFormat != source->format) {
            qCWarning(lcCodec) << "unsupported pixel format" << av_get_pix_fmt_name(format);
            m_lastLoggedFormat = source->format;
        }
        return {};
    }

    QVideoFrameFormat frameFormat(QSize(source->width, source->height), QVideoFrameFormat::Format_YUV420P);
    const bool fullRange = format == AV_PIX_FMT_YUVJ420P || source->color_range == AVCOL_RANGE_JPEG;
    frameFormat.setColorRange(fullRange ? QVideoFrameFormat::ColorRange_Full : QVideoFrameFormat::ColorRange_Video);
    frameFormat.setColorSpace(source->colorspace == AVCOL_SPC_BT709 ? QVideoFrameFormat::ColorSpace_BT709
                                                                   : QVideoFrameFormat::ColorSpace_BT601);

    QVideoFrame frame(frameFormat);
    if (!frame.map(QVideoFrame::WriteOnly))
        return {};
    for (int plane = 0; plane < 3; ++plane) {
        const int rows = plane == 0 ? source->height : (source->height + 1) / 2;
        const int rowBytes = plane == 0 ? source->width : (source->width + 1) / 2;
        av_image_copy_plane(frame.bits(plane), frame.bytesPerLine(plane), source->data[plane],
                            source->linesize[plane], rowBytes, rows);
    }
    frame.unmap();
    return frame;
}
