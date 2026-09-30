#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QTimer>
#include <QUdpSocket>
#include <QVariantList>

// Finds beamr receivers on the local network while the app is looking:
// broadcasts a query every couple of seconds and lists whoever answers,
// dropping receivers that go quiet. See beamr/protocol.h.
class DiscoveryClient : public QObject
{
    Q_OBJECT

public:
    explicit DiscoveryClient(QObject *parent = nullptr);

    void setActive(bool active);
    // [{name, address, freeScreens}], by name; address is host or host:port.
    QVariantList receivers() const;

signals:
    void receiversChanged();

private:
    struct Found
    {
        QString name;
        QString address;
        int freeScreens = 0;
        QElapsedTimer seen;
    };

    void query();
    void onReadyRead();
    void expire();

    QUdpSocket m_socket;
    QTimer m_timer;
    QHash<QString, Found> m_found;
};
