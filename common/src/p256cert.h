#pragma once

#include <QByteArray>

namespace beamr {

// A self-signed P-256 certificate and its SEC1 private key, both PEM.
// False when the key couldn't be made or Qt's TLS backend rejected the encoding.
bool createSelfSignedIdentity(QByteArray *certificatePem, QByteArray *privateKeyPem);

} // namespace beamr
