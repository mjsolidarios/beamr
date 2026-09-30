#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QObject>

class QTcpSocket;

// Sends every encoded frame to each receiver's video connection. Lives on
// its own thread so frames never wait behind the UI.
//
// TCP never drops data, so on a slow network frames would queue up and the
// picture would fall further and further behind. Instead, once too much is
// queued for a receiver we skip that receiver's frames until the next
// keyframe, which we ask the encoder for. Other receivers carry on.
class StreamSender : public QObject
{
    Q_OBJECT

public:
    explicit StreamSender(QObject *parent = nullptr);

    // All of these must be called on the sender's thread (queue them).
    // `audio`: the receiver plays sound, so send it along.
    void open(const QString &sessionId, const QString &host, quint16 port, const QString &token, bool audio);
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

private:
    struct Destination
    {
        QTcpSocket *socket = nullptr;
        bool audio = false;
        bool waitForKeyFrame = true;
        qint64 sentFrames = 0;
        qint64 droppedFrames = 0;
        qint64 droppedAudio = 0;
    };

    // Tears a connection down quietly; false if there was none.
    bool drop(const QString &sessionId);
    void send(Destination &destination, const QByteArray &header, const QByteArray &data, quint8 flags);

    QHash<QString, Destination> m_destinations;
    QElapsedTimer m_sinceKeyFrameRequest;
};
