#pragma once

#include <QObject>
#include <QUdpSocket>

class ReceiverController;

// Answers phones looking for receivers on the local network, so they can
// list this computer without scanning or typing. See beamr/protocol.h.
class DiscoveryResponder : public QObject
{
    Q_OBJECT

public:
    explicit DiscoveryResponder(ReceiverController *controller);

    bool listen(quint16 port);

private:
    void onReadyRead();

    ReceiverController *m_controller;
    QUdpSocket m_socket;
};
