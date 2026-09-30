#pragma once

#include <QString>
#include <QStringList>

namespace beamr {

// What a receiver's QR code carries: every address it can be reached at,
// since the sender picks the one on its own network, and which of the
// receiver's screens the code belongs to.
//
//   beamr://connect?name=Office%20PC&port=47700&screen=k3f9x2&pair=…&host=192.168.1.20&host=10.0.0.5
//
// `pair` is a one-time code: a phone that scanned it was in front of the
// screen, so the receiver lets it cast without asking.
struct ConnectLink
{
    QString name;
    QString screen;
    QString pair;
    QStringList hosts;
    quint16 port = 0;

    bool isValid() const { return !hosts.isEmpty() && port != 0; }

    QString toString() const;
    // Invalid when `text` isn't a beamr link.
    static ConnectLink parse(const QString &text);
};

} // namespace beamr
