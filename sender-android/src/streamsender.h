#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QObject>

class QSslSocket;

// Sends every encoded frame to each receiver's video connection. Lives on
// its own thread so frames never wait behind the UI.
//
// The TLS connection never drops data, so on a slow network frames would
// queue up and the picture would fall further and further behind. Instead,
// once too much is queued for a receiver we skip that receiver's frames
// until the next keyframe, which we ask the encoder for. Other receivers
// carry on.
class StreamSender : public QObject
{
    Q_OBJECT

public:
    explicit StreamSender(QObject *parent = nullptr);

    // All of these must be called on the sender's thread (queue them).
    // `audio`: the receiver plays sound, so send it along.
    // `fingerprint`: the certificate pinned on the control connection.
    void open(const QString &sessionId, const QString &host, quint16 port, const QString &token, bool audio,
              const QByteArray &fingerprint);
    void close(const QString &sessionId);
    void closeAll();
    void sendFrame(const QByteArray &data, quint8 flags, qint64 ptsUs);
    void sendAudio(const QByteArray &data, qint64 ptsUs);
    // Asks the encoder for a keyframe (at most every half second).
    void requestKeyFrame();

signals:
    void opened(const QString &sessionId);
    // Empty error when close() or closeAll() asked for it.
    void closed(const QString &sessionId, const QString &error);
    // A receiver's connection is skipping frames, or it has caught up.
    void congestionChanged(bool congested);

private:
    enum class SendResult { Ignored, Sent, Congested };

    struct Destination
    {
        QSslSocket *socket = nullptr;
        bool ready = false;
        bool audio = false;
        bool waitForKeyFrame = true;
        qint64 sentFrames = 0;
        qint64 droppedFrames = 0;
        qint64 droppedAudio = 0;
    };

    // Tears a connection down quietly; false if there was none.
    bool drop(const QString &sessionId);
    void fail(const QString &sessionId, QSslSocket *socket, const QString &error);
    SendResult send(Destination &destination, const QByteArray &header, const QByteArray &data, quint8 flags);
    // Counts congestion skips, and says when they amount to a struggling link.
    void noteDrop();
    void noteSent();
    void clearCongestion();

    QHash<QString, Destination> m_destinations;
    QElapsedTimer m_sinceKeyFrameRequest;
    QElapsedTimer m_clock;
    qint64 m_dropWindowStart = -1;
    qint64 m_lastDropMs = 0;
    int m_dropsInWindow = 0;
    bool m_congested = false;
};
