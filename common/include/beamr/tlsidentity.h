#pragma once

#include <QByteArray>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>
#include <QString>

class QSslSocket;

namespace beamr {

// The receiver's long-lived self-signed certificate. The private key stays
// on the computer; QR codes carry only `fingerprintText()`.
class TlsIdentity
{
public:
    TlsIdentity() = default;

    bool isValid() const { return !m_certificate.isNull() && !m_key.isNull(); }
    QSslCertificate certificate() const { return m_certificate; }
    QSslKey privateKey() const { return m_key; }
    QByteArray fingerprint() const { return fingerprintOf(m_certificate); }
    QString fingerprintText() const { return textFromFingerprint(fingerprint()); }

    // VerifyNone: phones pin the certificate instead of a public CA.
    QSslConfiguration serverConfiguration() const;
    static QSslConfiguration clientConfiguration();

    // Loads tls-cert.pem and tls-key.pem from `directory`, or writes a new pair.
    // Invalid when the directory can't be used or the certificate can't be made.
    static TlsIdentity loadOrCreate(const QString &directory);

    static QByteArray fingerprintOf(const QSslCertificate &certificate);
    // base64url, no padding. Empty when `text` isn't a 32-byte fingerprint.
    static QByteArray fingerprintFromText(const QString &text);
    static QString textFromFingerprint(const QByteArray &fingerprint);

    // True when the peer certificate matches `expected`, or when `expected`
    // is empty (trust this certificate, and report it in `seen`). A null
    // certificate fails either way. `seen` may be null.
    static bool pinPeer(const QSslSocket *socket, const QByteArray &expected, QByteArray *seen);

private:
    TlsIdentity(QSslCertificate certificate, QSslKey key);

    QSslCertificate m_certificate;
    QSslKey m_key;
};

} // namespace beamr
