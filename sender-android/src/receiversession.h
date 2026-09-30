#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

#include <beamr/device.h>

// The control connection to one receiver: says hello, waits for the person
// at that computer to allow it, and hands out the token for its video. The
// phone keeps one of these per receiver it casts to.
class ReceiverSession : public QObject
{
    Q_OBJECT

public:
    // Ready: allowed, no video yet. Streaming: video connection open.
    enum State { Connecting, AwaitingApproval, Ready, Streaming };
    Q_ENUM(State)

    // `screen` is the receiver screen to cast to, from its QR code; empty
    // lets the receiver pick.
    ReceiverSession(const QString &host, quint16 port, const QString &endpoint, const beamr::DeviceInfo &device,
                    const QString &screen, QObject *parent = nullptr);

    QString id() const { return m_id; }
    QString host() const { return m_host; }
    quint16 port() const { return m_port; }
    // As the user typed it: host, or host:port when not the default.
    QString endpoint() const { return m_endpoint; }
    // The receiver's own name once it has said hello, the endpoint before.
    QString name() const { return m_name; }
    QString streamToken() const { return m_streamToken; }
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
    // The session is over and this object can go.
    void ended(const QString &message, bool isError);

private:
    void setState(State state);
    void end(const QString &message, bool isError);
    void onConnected();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onDisconnected();

    const QString m_id;
    const QString m_host;
    const quint16 m_port;
    const QString m_endpoint;
    const beamr::DeviceInfo m_device;
    const QString m_screen;
    QString m_name;
    QString m_streamToken;
    State m_state = Connecting;
    bool m_ended = false;
    QTcpSocket m_socket;
    QTimer m_timeout;
};
