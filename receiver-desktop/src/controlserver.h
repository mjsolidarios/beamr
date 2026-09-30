#pragma once

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QTcpServer>

class QTcpSocket;
class ReceiverController;

// Accepts senders on the control port and turns their hello into a
// connection request; relays the user's answer and the end of a cast back
// to the phone. See beamr/protocol.h for the wire format.
class ControlServer : public QObject
{
    Q_OBJECT

public:
    explicit ControlServer(ReceiverController *controller);

    bool listen(quint16 port);
    QString errorString() const { return m_server.errorString(); }

private:
    void onNewConnection();
    void onReadyRead(QTcpSocket *socket);
    void onDisconnected(QTcpSocket *socket);
    void handleHello(QTcpSocket *socket, const QJsonObject &hello);
    void handleStream(QTcpSocket *socket, const QJsonObject &stream);
    void readFrames(QTcpSocket *socket);
    void answer(const QString &requestId, bool accepted, const char *reason);
    void close(const QString &requestId, const QJsonObject &lastMessage);

    ReceiverController *m_controller;
    QTcpServer m_server;
    // Senders that said hello, by request id. Sockets that haven't are only
    // parented to the server.
    QHash<QString, QTcpSocket *> m_peers;
    // Video connections, by request id, and the tokens that open them.
    QHash<QString, QTcpSocket *> m_media;
    QHash<QString, QString> m_streamTokens;
};
