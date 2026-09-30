#include "discoveryresponder.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDatagram>

#include <beamr/log.h>
#include <beamr/protocol.h>

#include "receivercontroller.h"

namespace protocol = beamr::protocol;

DiscoveryResponder::DiscoveryResponder(ReceiverController *controller)
    : QObject(controller)
    , m_controller(controller)
{
    connect(&m_socket, &QUdpSocket::readyRead, this, &DiscoveryResponder::onReadyRead);
}

bool DiscoveryResponder::listen(quint16 port)
{
    // Shared, so a second receiver on the same computer (BEAMR_PORT) can
    // answer too.
    if (!m_socket.bind(QHostAddress::AnyIPv4, port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint))
        return false;
    qCInfo(lcNet) << "answering discovery on UDP port" << port;
    return true;
}

void DiscoveryResponder::onReadyRead()
{
    while (m_socket.hasPendingDatagrams()) {
        const QNetworkDatagram datagram = m_socket.receiveDatagram(512);
        const QJsonObject query = QJsonDocument::fromJson(datagram.data().trimmed()).object();
        if (query.value("type").toString() != QLatin1StringView(protocol::kDiscover))
            continue;

        int freeScreens = 0;
        for (const CastScreen *screen : m_controller->screens()) {
            if (!screen->casting())
                ++freeScreens;
        }
        const QJsonObject reply{
            {"type", protocol::kReceiver},
            {"version", protocol::kVersion},
            {"name", m_controller->receiverName()},
            {"port", m_controller->port()},
            {"freeScreens", freeScreens},
        };
        m_socket.writeDatagram(QJsonDocument(reply).toJson(QJsonDocument::Compact), datagram.senderAddress(),
                               quint16(datagram.senderPort()));
    }
}
