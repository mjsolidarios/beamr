#pragma once

#include <QString>

namespace beamr {

// Identity a sender presents when it asks to cast. The id is a random UUID
// the phone generates once per install, so trust survives IP changes.
struct DeviceInfo
{
    QString id;
    QString name;  // user-visible, e.g. "Alex's Pixel"
    QString model; // e.g. "Google Pixel 8 Pro"
};

} // namespace beamr
