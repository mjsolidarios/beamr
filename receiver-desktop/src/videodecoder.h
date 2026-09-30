#pragma once

#include <QObject>
#include <QVideoFrame>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;

// Turns the sender's H.264 into frames Qt Multimedia can draw. Lives on a
// worker thread; call its slots queued.
class VideoDecoder : public QObject
{
    Q_OBJECT

public:
    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder() override;

    // `generation` comes back with each frame, so frames from a stream that
    // has since ended can be told apart.
    void decode(const QByteArray &packet, int generation);
    // Forgets the current stream; the next one starts from its first keyframe.
    void reset();

signals:
    void frameDecoded(const QVideoFrame &frame, int generation);

private:
    bool open();
    void close();
    QVideoFrame toVideoFrame(const AVFrame *frame) const;

    AVCodecContext *m_context = nullptr;
    AVPacket *m_packet = nullptr;
    AVFrame *m_frame = nullptr;
    bool m_failed = false;
    mutable int m_lastLoggedFormat = -1;
};
