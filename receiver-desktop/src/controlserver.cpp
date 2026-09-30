#include "controlserver.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonValue>
#include <QPointer>
#include <QTcpSocket>
#include <QTimer>
#include <QUuid>

#include <beamr/log.h>
#include <beamr/protocol.h>

#include "receivercontroller.h"

namespace protocol = beamr::protocol;

namespace {

const char kRequestIdProperty[] = "beamrRequestId";
const char kMediaProperty[] = "beamrMedia";

QString requestIdOf(const QTcpSocket *socket)
{
    return socket->property(kRequestIdProperty).toString();
}

bool isMedia(const QTcpSocket *socket)
{
    return socket->property(kMediaProperty).toBool();
}

// Dual-stack sockets report IPv4 peers as ::ffff:a.b.c.d.
QString displayAddress(const QHostAddress &address)
{
    bool isV4 = false;
    const quint32 v4 = address.toIPv4Address(&isV4);
    return isV4 ? QHostAddress(v4).toString() : address.toString();
}

void send(QTcpSocket *socket, const QJsonObject &message)
{
    socket->write(protocol::encode(message));
}

} // namespace

ControlServer::ControlServer(ReceiverController *controller)
    : QObject(controller)
    , m_controller(controller)
{
    connect(&m_server, &QTcpServer::newConnection, this, &ControlServer::onNewConnection);

    connect(controller, &ReceiverController::requestAnswered, this, [this](const QString &requestId, bool accepted) {
        answer(requestId, accepted, protocol::kReasonDeclined);
    });
    connect(controller, &ReceiverController::requestExpired, this, [this](const QString &requestId) {
        answer(requestId, false, protocol::kReasonExpired);
    });
    connect(controller, &ReceiverController::castStopped, this, [this](const QString &requestId) {
        close(requestId, {{"type", protocol::kBye}});
    });
}

bool ControlServer::listen(quint16 port)
{
    if (!m_server.listen(QHostAddress::Any, port))
        return false;
    qCInfo(lcNet) << "control server listening on port" << port;
    return true;
}

void ControlServer::onNewConnection()
{
    while (QTcpSocket *socket = m_server.nextPendingConnection()) {
        socket->setParent(this);
        qCInfo(lcNet) << "sender connected from" << displayAddress(socket->peerAddress());

        connect(socket, &QTcpSocket::readyRead, this, [this, socket] { onReadyRead(socket); });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket] { onDisconnected(socket); });

        // Port scanners and stray clients never say hello; don't keep them.
        QTimer::singleShot(protocol::kHelloTimeoutMs, socket, [socket] {
            if (requestIdOf(socket).isEmpty())
                socket->abort();
        });
    }
}

void ControlServer::onReadyRead(QTcpSocket *socket)
{
    QPointer<QTcpSocket> guard(socket);
    for (;;) {
        if (isMedia(socket)) {
            readFrames(socket);
            return;
        }

        bool malformed = false;
        const std::optional<QJsonObject> message = protocol::readMessage(socket, &malformed);
        if (malformed) {
            qCWarning(lcNet) << "malformed message from" << displayAddress(socket->peerAddress());
            socket->abort();
            return;
        }
        if (!message)
            return;

        const QString type = message->value("type").toString();
        if (type == QLatin1StringView(protocol::kHello) && requestIdOf(socket).isEmpty()) {
            handleHello(socket, *message);
        } else if (type == QLatin1StringView(protocol::kStream) && requestIdOf(socket).isEmpty()) {
            handleStream(socket, *message);
        } else if (type == QLatin1StringView(protocol::kBye)) {
            socket->disconnectFromHost();
            return;
        }
        // Rejecting a hello or stream closes the socket; stop reading from it.
        if (!guard || socket->state() != QAbstractSocket::ConnectedState)
            return;
    }
}

void ControlServer::handleHello(QTcpSocket *socket, const QJsonObject &hello)
{
    const beamr::DeviceInfo device{
        hello.value("deviceId").toString().left(64),
        hello.value("name").toString().trimmed().left(64),
        hello.value("model").toString().trimmed().left(64),
    };

    if (hello.value("version").toInt() != protocol::kVersion || device.id.isEmpty()) {
        qCWarning(lcNet) << "rejecting sender with protocol version" << hello.value("version").toInt();
        send(socket, {{"type", protocol::kAnswer}, {"accepted", false}, {"reason", protocol::kReasonUnsupported}});
        socket->disconnectFromHost();
        return;
    }

    const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    socket->setProperty(kRequestIdProperty, requestId);
    m_peers.insert(requestId, socket);

    send(socket, {{"type", protocol::kWelcome},
                  {"name", m_controller->receiverName()},
                  {"audio", QJsonArray{protocol::kCodecOpus}}});
    m_controller->handleIncomingRequest(requestId, device, displayAddress(socket->peerAddress()),
                                        hello.value("screen").toString().left(32));
}

void ControlServer::handleStream(QTcpSocket *socket, const QJsonObject &stream)
{
    const QString requestId = m_streamTokens.value(stream.value("token").toString());
    if (requestId.isEmpty() || !m_controller->isCasting(requestId)
        || stream.value("codec").toString() != QLatin1StringView(protocol::kCodecH264)) {
        qCWarning(lcNet) << "rejecting video stream from" << displayAddress(socket->peerAddress());
        socket->abort();
        return;
    }

    socket->setProperty(kRequestIdProperty, requestId);
    socket->setProperty(kMediaProperty, true);
    socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    // A reconnect replaces the old stream.
    if (QTcpSocket *previous = m_media.value(requestId); previous && previous != socket)
        previous->abort();
    m_media.insert(requestId, socket);
    qCInfo(lcNet) << "video stream from" << displayAddress(socket->peerAddress());
}

void ControlServer::readFrames(QTcpSocket *socket)
{
    const QString requestId = requestIdOf(socket);
    while (socket->bytesAvailable() >= protocol::FrameHeader::kSize) {
        char headerBytes[protocol::FrameHeader::kSize];
        socket->peek(headerBytes, sizeof headerBytes);
        const protocol::FrameHeader header = protocol::FrameHeader::decode(headerBytes);
        if (header.size == 0 || header.size > protocol::FrameHeader::kMaxFrameBytes) {
            qCWarning(lcNet) << "bad video frame size" << header.size << "; dropping the stream";
            socket->abort();
            return;
        }
        if (socket->bytesAvailable() < protocol::FrameHeader::kSize + qint64(header.size))
            return;
        socket->skip(protocol::FrameHeader::kSize);
        if (header.flags & protocol::kFrameAudio)
            m_controller->audioPacket(requestId, socket->read(header.size));
        else
            m_controller->videoPacket(requestId, socket->read(header.size));
    }
}

void ControlServer::onDisconnected(QTcpSocket *socket)
{
    const QString requestId = requestIdOf(socket);
    if (isMedia(socket)) {
        if (m_media.value(requestId) == socket) {
            m_media.remove(requestId);
            qCInfo(lcNet) << "video stream ended";
            m_controller->videoEnded(requestId);
        }
        socket->deleteLater();
        return;
    }
    // Still in m_peers means the phone hung up, not us.
    if (!requestId.isEmpty() && m_peers.remove(requestId)) {
        qCInfo(lcNet) << "sender left" << displayAddress(socket->peerAddress());
        m_controller->senderLeft(requestId);
    }
    socket->deleteLater();
}

void ControlServer::answer(const QString &requestId, bool accepted, const char *reason)
{
    if (accepted) {
        if (QTcpSocket *socket = m_peers.value(requestId)) {
            const QString token = QUuid::createUuid().toString(QUuid::Id128);
            m_streamTokens.insert(token, requestId);
            send(socket, {{"type", protocol::kAnswer}, {"accepted", true}, {"streamToken", token}});
        }
        return;
    }
    close(requestId, {{"type", protocol::kAnswer}, {"accepted", false}, {"reason", reason}});
}

void ControlServer::close(const QString &requestId, const QJsonObject &lastMessage)
{
    m_streamTokens.removeIf([&](const std::pair<const QString &, QString &> &entry) {
        return entry.second == requestId;
    });
    if (QTcpSocket *media = m_media.take(requestId))
        media->disconnectFromHost();

    QTcpSocket *socket = requestId.isEmpty() ? nullptr : m_peers.take(requestId);
    if (!socket)
        return;
    send(socket, lastMessage);
    // Flushes the message before closing.
    socket->disconnectFromHost();
}
