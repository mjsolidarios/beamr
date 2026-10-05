#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QSslSocket>
#include <QTimer>

#include <beamr/device.h>

// The control connection to one receiver: says hello over TLS, waits for the
// person at that computer to allow it, and hands out the token for its video.
// The phone keeps one of these per receiver it casts to.
class ReceiverSession : public QObject
{
    Q_OBJECT

public:
    // Ready: allowed, no video yet. Streaming: video connection open.
    // Reconnecting: the connection dropped; trying to get back in.
    enum State { Connecting, AwaitingApproval, Ready, Streaming, Reconnecting };
    Q_ENUM(State)

    // `screen` is the receiver screen to cast to and `pair` its one-time
    // code, both from its QR code; empty lets the receiver pick and ask.
    // `fingerprint` is that code's certificate fingerprint. Empty pins the
    // certificate this connection sees and requires it again for the video.
    ReceiverSession(const QString &host, quint16 port, const QString &endpoint, const beamr::DeviceInfo &device,
                    const QString &screen, const QString &pair, const QByteArray &fingerprint,
                    QObject *parent = nullptr);

    QString id() const { return m_id; }
    QString host() const { return m_host; }
    quint16 port() const { return m_port; }
    // As the user typed it: host, or host:port when not the default.
    QString endpoint() const { return m_endpoint; }
    // The receiver's own name once it has said hello, the endpoint before.
    QString name() const { return m_name; }
    QString streamToken() const { return m_streamToken; }
    // Certificate pinned for this receiver, empty until the handshake when
    // the QR code didn't carry one.
    QByteArray peerFingerprint() const { return m_fingerprint; }
    // The receiver said it plays our sound (Opus).
    bool playsAudio() const { return m_playsAudio; }
    State state() const { return m_state; }
    bool isApproved() const { return m_state >= Ready; }

    void start();
    // Says goodbye; emits nothing.
    void close();
    void setStreaming(bool streaming);

signals:
    void changed();
    // It's a beamr receiver; worth remembering.
    void welcomed();
    void approved();
    // The receiver wants a keyframe, e.g. to start a recording cleanly.
    void keyFrameRequested();
    // The session is over and this object can go.
    void ended(const QString &message, bool isError);

private:
    void setState(State state);
    void end(const QString &message, bool isError);
    void connectEncrypted();
    void onEncrypted();
    void sendHello();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onDisconnected();
    // After a drop, if the receiver gave us a way back: retry until it lets
    // us in or its grace period runs out.
    bool beginReconnect();
    void retry();

    const QString m_id;
    const QString m_host;
    const quint16 m_port;
    const QString m_endpoint;
    const beamr::DeviceInfo m_device;
    const QString m_screen;
    const QString m_pair;
    QByteArray m_fingerprint;
    QString m_name;
    QString m_streamToken;
    QString m_resume;
    bool m_playsAudio = false;
    State m_state = Connecting;
    bool m_ended = false;
    QSslSocket m_socket;
    QTimer m_timeout;
    QTimer m_retryTimer;
    QElapsedTimer m_sinceDrop;
};
