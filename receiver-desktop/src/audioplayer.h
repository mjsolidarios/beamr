#pragma once

#include <QObject>

#include <memory>

class QAudioSink;
class QIODevice;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;

// Decodes a phone's Opus sound and plays it on the default output. Lives on
// its screen's decoder thread; call its slots queued.
//
// Sound is played as it arrives, with about a tenth of a second of buffer:
// enough to ride out Wi-Fi jitter, close enough to keep up with the picture.
// When more piles up, the excess is dropped so it never drifts behind.
class AudioPlayer : public QObject
{
    Q_OBJECT

public:
    explicit AudioPlayer(QObject *parent = nullptr);
    ~AudioPlayer() override;

    void decode(const QByteArray &packet);
    void setMuted(bool muted);
    // Stops playback and forgets the stream; the next packet starts afresh.
    void reset();

private:
    bool openDecoder();
    void closeDecoder();
    bool openOutput();
    void closeOutput();

    AVCodecContext *m_context = nullptr;
    AVPacket *m_packet = nullptr;
    AVFrame *m_frame = nullptr;
    bool m_failed = false;

    std::unique_ptr<QAudioSink> m_sink;
    QIODevice *m_output = nullptr;
    bool m_muted = false;
    QByteArray m_pcm;
};
