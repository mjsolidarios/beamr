#pragma once

#include <QByteArray>
#include <QObject>
#include <QSize>
#include <QString>

struct AVFormatContext;
struct AVStream;

// Writes one screen's cast to an MP4 as it arrives: the phone's H.264 and
// Opus packets go into the file untouched, so recording costs no CPU and no
// quality. Lives on its screen's decoder thread; call its slots queued.
//
// The file starts at the first keyframe (with the SPS/PPS keyframes carry).
// While paused nothing is written, and afterwards it waits for the next
// keyframe again, with the gap taken out of the timeline so the file plays
// straight through.
class Recorder : public QObject
{
    Q_OBJECT

public:
    explicit Recorder(QObject *parent = nullptr);
    ~Recorder() override;

    // `videoSize` is the decoded picture size, for the file's header.
    void start(const QString &path, QSize videoSize, bool withAudio);
    void setPaused(bool paused);
    void video(const QByteArray &packet, quint8 flags, qint64 ptsUs);
    void audio(const QByteArray &packet, qint64 ptsUs);
    void stop();

signals:
    // Empty error on success; either way the file is closed.
    void finished(const QString &path, const QString &error);

private:
    bool open(const QByteArray &keyframe);
    void write(AVStream *stream, const QByteArray &data, qint64 ptsUs, bool key);
    void fail(const QString &error);
    void close();

    QString m_path;
    QSize m_videoSize;
    bool m_withAudio = false;
    bool m_active = false;
    bool m_paused = false;
    // Waiting for a keyframe: at the start, and after a pause.
    bool m_needKey = true;

    AVFormatContext *m_format = nullptr;
    AVStream *m_videoStream = nullptr;
    AVStream *m_audioStream = nullptr;
    // Phone time → file time: the first packet's time, plus every pause.
    qint64 m_originUs = -1;
    qint64 m_pausedAtUs = -1;
    qint64 m_lastVideoTicks = -1;
    qint64 m_lastAudioTicks = -1;
    qint64 m_lastInputUs = 0;
};
