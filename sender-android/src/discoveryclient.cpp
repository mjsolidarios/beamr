#include "discoveryclient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkDatagram>
#include <QNetworkInterface>

#include <algorithm>

#include <beamr/config.h>
#include <beamr/log.h>
#include <beamr/protocol.h>

namespace protocol = beamr::protocol;

namespace {

constexpr int kQueryIntervalMs = 2'000;
// Three missed answers and it's gone.
constexpr int kForgetAfterMs = 7'000;

} // namespace

DiscoveryClient::DiscoveryClient(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(kQueryIntervalMs);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        expire();
        query();
    });
    connect(&m_socket, &QUdpSocket::readyRead, this, &DiscoveryClient::onReadyRead);
}

void DiscoveryClient::setActive(bool active)
{
    if (active == m_timer.isActive())
        return;
    if (active) {
        if (m_socket.state() != QAbstractSocket::BoundState)
            m_socket.bind(QHostAddress::AnyIPv4, 0);
        query();
        m_timer.start();
    } else {
        m_timer.stop();
    }
}

void DiscoveryClient::query()
{
    const QByteArray message = QJsonDocument(QJsonObject{
        {"type", protocol::kDiscover},
        {"version", protocol::kVersion},
    }).toJson(QJsonDocument::Compact);

    // Each network's own broadcast address; some routers drop the global one.
    QList<QHostAddress> targets{QHostAddress::Broadcast};
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        if (!iface.flags().testFlag(QNetworkInterface::IsUp) || iface.flags().testFlag(QNetworkInterface::IsLoopBack))
            continue;
        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.broadcast().isNull()
                && !targets.contains(entry.broadcast())) {
                targets.append(entry.broadcast());
            }
        }
    }
    for (const QHostAddress &target : std::as_const(targets))
        m_socket.writeDatagram(message, target, beamr::kDiscoveryPort);
}

void DiscoveryClient::onReadyRead()
{
    bool changed = false;
    while (m_socket.hasPendingDatagrams()) {
        const QNetworkDatagram datagram = m_socket.receiveDatagram(1024);
        const QJsonObject reply = QJsonDocument::fromJson(datagram.data()).object();
        if (reply.value("type").toString() != QLatin1StringView(protocol::kReceiver))
            continue;

        bool isV4 = false;
        const quint32 v4 = datagram.senderAddress().toIPv4Address(&isV4);
        const QString host = isV4 ? QHostAddress(v4).toString() : datagram.senderAddress().toString();
        const int port = reply.value("port").toInt(beamr::kDefaultControlPort);
        const QString address = port == beamr::kDefaultControlPort ? host : host + u':' + QString::number(port);

        Found &found = m_found[address];
        const QString name = reply.value("name").toString().trimmed().left(64);
        const int free = reply.value("freeScreens").toInt();
        if (found.address.isEmpty() || found.name != name || found.freeScreens != free)
            changed = true;
        found.address = address;
        found.name = name.isEmpty() ? address : name;
        found.freeScreens = free;
        found.seen.start();
    }
    if (changed)
        emit receiversChanged();
}

void DiscoveryClient::expire()
{
    const auto removed = m_found.removeIf([](const auto &entry) {
        return entry.value().seen.elapsed() > kForgetAfterMs;
    });
    if (removed > 0)
        emit receiversChanged();
}

QVariantList DiscoveryClient::receivers() const
{
    QList<Found> sorted = m_found.values();
    std::sort(sorted.begin(), sorted.end(), [](const Found &a, const Found &b) {
        return a.name.localeAwareCompare(b.name) < 0;
    });
    QVariantList list;
    for (const Found &found : sorted) {
        list.append(QVariantMap{{"name", found.name}, {"address", found.address}, {"freeScreens", found.freeScreens}});
    }
    return list;
}
