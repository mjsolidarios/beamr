#include "p256cert.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QSslCertificate>
#include <QSslKey>

#include <uECC.h>

namespace beamr {

namespace {

QByteArray der(quint8 tag, const QByteArray &body)
{
    QByteArray out;
    out.append(char(tag));
    const int n = body.size();
    if (n < 0x80) {
        out.append(char(n));
    } else if (n <= 0xff) {
        out.append(char(0x81));
        out.append(char(n));
    } else if (n <= 0xffff) {
        out.append(char(0x82));
        out.append(char((n >> 8) & 0xff));
        out.append(char(n & 0xff));
    } else {
        return {};
    }
    out.append(body);
    return out;
}

QByteArray seq(const QByteArray &body) { return der(0x30, body); }

QByteArray oid(const char *hex) { return der(0x06, QByteArray::fromHex(hex)); }

// Unsigned big-endian magnitude, encoded as a positive INTEGER.
QByteArray derInteger(const QByteArray &magnitude)
{
    int start = 0;
    while (start < magnitude.size() - 1 && magnitude.at(start) == 0)
        ++start;
    QByteArray body = magnitude.mid(start);
    if (body.isEmpty())
        body.append('\0');
    if (static_cast<unsigned char>(body.at(0)) & 0x80)
        body.prepend('\0');
    return der(0x02, body);
}

QByteArray bitString(const QByteArray &payload)
{
    return der(0x03, QByteArray(1, '\0') + payload);
}

QByteArray octetString(const QByteArray &body) { return der(0x04, body); }

QByteArray booleanTrue() { return der(0x01, QByteArray(1, '\xff')); }

QByteArray utcTime(const QDateTime &time)
{
    const QByteArray text = time.toUTC().toString(QStringLiteral("yyMMddHHmmss")).toLatin1() + 'Z';
    return der(0x17, text);
}

QByteArray commonName()
{
    const QByteArray atv = seq(oid("550403") + der(0x13, QByteArray("beamr")));
    return seq(der(0x31, atv));
}

QByteArray extension(const QByteArray &id, bool critical, const QByteArray &value)
{
    QByteArray body = id;
    if (critical)
        body += booleanTrue();
    body += octetString(value);
    return seq(body);
}

QByteArray pem(const char *label, const QByteArray &body)
{
    const QByteArray encoded = body.toBase64();
    QByteArray out = QByteArray("-----BEGIN ") + label + "-----\n";
    for (int i = 0; i < encoded.size(); i += 64) {
        out += encoded.mid(i, 64);
        out += '\n';
    }
    out += QByteArray("-----END ") + label + "-----\n";
    return out;
}

} // namespace

bool createSelfSignedIdentity(QByteArray *certificatePem, QByteArray *privateKeyPem)
{
    if (!certificatePem || !privateKeyPem)
        return false;
    *certificatePem = {};
    *privateKeyPem = {};

    // Qt links OpenSSL on Linux, Windows and Android, and Secure Transport on
    // macOS. None of those expose a portable way to mint a certificate, so
    // the key and the X.509 encoding live here and TLS itself stays in Qt.
    const uECC_RNG_Function rng = uECC_get_rng();
    const uECC_Curve curve = uECC_secp256r1();
    if (!rng || !curve || uECC_curve_private_key_size(curve) != 32 || uECC_curve_public_key_size(curve) != 64)
        return false;

    uint8_t pub[64];
    uint8_t priv[32];
    if (uECC_make_key(pub, priv, curve) != 1)
        return false;

    uint8_t serialRaw[8];
    if (rng(serialRaw, sizeof serialRaw) != 1)
        return false;
    bool any = false;
    for (unsigned char byte : serialRaw)
        any = any || byte != 0;
    if (!any)
        serialRaw[7] = 1;

    const QByteArray point = QByteArray(1, '\x04') + QByteArray(reinterpret_cast<const char *>(pub), 64);
    const QByteArray sigAlg = seq(oid("2A8648CE3D040302")); // ecdsa-with-SHA256
    const QByteArray spki = seq(seq(oid("2A8648CE3D0201") + oid("2A8648CE3D030107")) + bitString(point));

    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QByteArray keyUsage = der(0x03, QByteArray::fromHex("0780")); // digitalSignature
    const QByteArray extensions = der(0xA3, seq(
        extension(oid("551D0F"), true, keyUsage)
        + extension(oid("551D13"), true, seq(QByteArray()))
        + extension(oid("551D25"), false, seq(oid("2B06010505070301")))));

    const QByteArray tbs = seq(der(0xA0, derInteger(QByteArray(1, '\x02')))
                               + derInteger(QByteArray(reinterpret_cast<const char *>(serialRaw), 8))
                               + sigAlg
                               + commonName()
                               + seq(utcTime(now.addDays(-1)) + utcTime(now.addYears(10)))
                               + commonName()
                               + spki
                               + extensions);

    const QByteArray digest = QCryptographicHash::hash(tbs, QCryptographicHash::Sha256);
    uint8_t signature[64];
    if (uECC_sign(priv, reinterpret_cast<const uint8_t *>(digest.constData()), unsigned(digest.size()), signature, curve) != 1)
        return false;
    if (uECC_verify(pub, reinterpret_cast<const uint8_t *>(digest.constData()), unsigned(digest.size()), signature, curve) != 1)
        return false;

    const QByteArray r(reinterpret_cast<const char *>(signature), 32);
    const QByteArray s(reinterpret_cast<const char *>(signature + 32), 32);
    const QByteArray certDer = seq(tbs + sigAlg + bitString(seq(derInteger(r) + derInteger(s))));

    // SEC1 (RFC 5915). QSslKey reads this PEM on every Qt TLS backend.
    const QByteArray keyDer = seq(derInteger(QByteArray(1, '\x01'))
                                  + octetString(QByteArray(reinterpret_cast<const char *>(priv), 32))
                                  + der(0xA0, oid("2A8648CE3D030107"))
                                  + der(0xA1, bitString(point)));

    const QByteArray certPem = pem("CERTIFICATE", certDer);
    const QByteArray keyPem = pem("EC PRIVATE KEY", keyDer);
    const QSslCertificate certificate(certPem, QSsl::Pem);
    const QSslKey key(keyPem, QSsl::Ec, QSsl::Pem, QSsl::PrivateKey);
    if (certificate.isNull() || key.isNull() || certificate.publicKey().algorithm() != QSsl::Ec) {
        qWarning("beamr: Qt rejected the generated TLS certificate");
        return false;
    }

    *certificatePem = certPem;
    *privateKeyPem = keyPem;
    return true;
}

} // namespace beamr
