#include "receiversession.h"

#include <QJsonObject>
#include <QUuid>

#include <beamr/config.h>
#include <beamr/log.h>
#include <beamr/protocol.h>

namespace protocol = beamr::protocol;

namespace {

constexpr int kConnectTimeoutMs = 8'000;
// The receiver expires a request after 60 s and says so; this only catches a
// receiver that went quiet.
constexpr int kApprovalTimeoutMs = 75'000;

} // namespace

ReceiverSession::ReceiverSession(const QString &host, quint16 port, const QString &endpoint,
                                 const beamr::DeviceInfo &device, const QString &screen, QObject *parent)
    : QObject(parent)
    , m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_host(host)
    , m_port(port)
    , m_endpoint(endpoint)
    , m_device(device)
    , m_screen(screen)
    , m_name(endpoint)
{
    m_timeout.setSingleShot(true);
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        if (m_state == Connecting) {
            end(tr("Couldn't reach %1. Check that your phone and the computer are on the same Wi-Fi "
                   "and that the computer's firewall allows port %2.")
                    .arg(m_endpoint)
                    .arg(beamr::kDefaultControlPort),
                true);
        } else if (m_state == AwaitingApproval) {
            end(tr("“%1” didn't answer. Try again.").arg(m_name), true);
        }
    });

    connect(&m_socket, &QTcpSocket::connected, this, &ReceiverSession::onConnected);
    connect(&m_socket, &QTcpSocket::readyRead, this, &ReceiverSession::onReadyRead);
    connect(&m_socket, &QTcpSocket::errorOccurred, this, &ReceiverSession::onSocketError);
    connect(&m_socket, &QTcpSocket::disconnected, this, &ReceiverSession::onDisconnected);
}

void ReceiverSession::start()
{
    qCInfo(lcNet) << "connecting to" << m_host << m_port;
    m_timeout.start(kConnectTimeoutMs);
    m_socket.connectToHost(m_host, m_port);
}

void ReceiverSession::close()
{
    m_ended = true;
    m_timeout.stop();
    if (m_socket.state() == QAbstractSocket::ConnectedState) {
        m_socket.write(protocol::encode({{"type", protocol::kBye}}));
        m_socket.disconnectFromHost();
    } else {
        m_socket.abort();
    }
}

void ReceiverSession::setStreaming(bool streaming)
{
    if (!isApproved())
        return;
    setState(streaming ? Streaming : Ready);
}

void ReceiverSession::setState(State state)
{
    if (state == m_state)
        return;
    m_state = state;
    emit changed();
}

void ReceiverSession::end(const QString &message, bool isError)
{
    if (m_ended)
        return;
    qCInfo(lcNet) << "session with" << m_endpoint << "ended:" << message;
    m_ended = true;
    m_timeout.stop();
    m_socket.abort();
    emit ended(message, isError);
}

void ReceiverSession::onConnected()
{
    // Waiting for the welcome still counts as connecting: a non-beamr service
    // on this port accepts the TCP connection but never answers.
    QJsonObject hello{
        {"type", protocol::kHello},
        {"version", protocol::kVersion},
        {"deviceId", m_device.id},
        {"name", m_device.name},
        {"model", m_device.model},
    };
    if (!m_screen.isEmpty())
        hello.insert("screen", m_screen);
    m_socket.write(protocol::encode(hello));
}

void ReceiverSession::onReadyRead()
{
    while (!m_ended) {
        bool malformed = false;
        const std::optional<QJsonObject> message = protocol::readMessage(&m_socket, &malformed);
        if (malformed) {
            end(tr("%1 isn't a beamr receiver.").arg(m_endpoint), true);
            return;
        }
        if (!message)
            return;

        const QString type = message->value("type").toString();
        if (type == QLatin1StringView(protocol::kWelcome)) {
            const QString name = message->value("name").toString().trimmed();
            m_name = name.isEmpty() ? m_endpoint : name;
            m_timeout.start(kApprovalTimeoutMs);
            setState(AwaitingApproval);
            emit changed();
            emit welcomed();
        } else if (type == QLatin1StringView(protocol::kAnswer)) {
            if (message->value("accepted").toBool()) {
                m_timeout.stop();
                m_streamToken = message->value("streamToken").toString();
                setState(Ready);
                emit approved();
                continue;
            }
            const QString reason = message->value("reason").toString();
            if (reason == QLatin1StringView(protocol::kReasonExpired))
                end(tr("Nobody answered on “%1” in time. Try again, then allow the request on the computer.")
                        .arg(m_name),
                    true);
            else if (reason == QLatin1StringView(protocol::kReasonUnsupported))
                end(tr("“%1” runs a different version of beamr. Update both apps, then try again.").arg(m_name),
                    true);
            else
                end(tr("“%1” declined the request.").arg(m_name), true);
        } else if (type == QLatin1StringView(protocol::kBye)) {
            end(tr("“%1” ended the session.").arg(m_name), false);
        }
    }
}

void ReceiverSession::onSocketError(QAbstractSocket::SocketError error)
{
    // A closed connection is handled in onDisconnected().
    if (m_ended || error == QAbstractSocket::RemoteHostClosedError)
        return;

    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
        end(tr("Nothing answered at %1. Make sure beamr is open on the computer and the address is right.")
                .arg(m_endpoint),
            true);
        break;
    case QAbstractSocket::HostNotFoundError:
        end(tr("Couldn't find %1. Check the address.").arg(m_endpoint), true);
        break;
    case QAbstractSocket::NetworkError:
    case QAbstractSocket::SocketTimeoutError:
        end(tr("Couldn't reach %1. Check that your phone and the computer are on the same Wi-Fi.").arg(m_endpoint),
            true);
        break;
    default:
        end(tr("Couldn't connect to %1: %2").arg(m_endpoint, m_socket.errorString()), true);
        break;
    }
}

void ReceiverSession::onDisconnected()
{
    if (m_state == Connecting)
        end(tr("%1 closed the connection. Is it a beamr receiver?").arg(m_endpoint), true);
    else
        end(tr("Lost the connection to “%1”.").arg(m_name), true);
}
