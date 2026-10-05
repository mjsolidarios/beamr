#include <beamr/tlsidentity.h>

#include "p256cert.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QSaveFile>
#include <QSslSocket>

#include <beamr/log.h>

namespace beamr {

namespace {

const QString kCertFile = QStringLiteral("tls-cert.pem");
const QString kKeyFile = QStringLiteral("tls-key.pem");

QSslConfiguration baseConfiguration()
{
    QSslConfiguration configuration = QSslConfiguration::defaultConfiguration();
    configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
    configuration.setProtocol(QSsl::TlsV1_2OrLater);
    return configuration;
}

bool usable(const QSslCertificate &certificate, const QSslKey &key)
{
    if (certificate.isNull() || key.isNull())
        return false;
    if (key.type() != QSsl::PrivateKey || key.algorithm() != QSsl::Ec)
        return false;
    if (certificate.publicKey().algorithm() != QSsl::Ec)
        return false;
    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QDateTime expiry = certificate.expiryDate();
    if (!expiry.isValid() || expiry <= now.addDays(30))
        return false;
    const QDateTime starts = certificate.effectiveDate();
    if (starts.isValid() && starts > now.addDays(2))
        return false;
    return TlsIdentity::fingerprintOf(certificate).size() == 32;
}

bool writePrivate(const QString &path, const QByteArray &bytes)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    if (file.write(bytes) != bytes.size())
        return false;
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return file.commit();
}

} // namespace

TlsIdentity::TlsIdentity(QSslCertificate certificate, QSslKey key)
    : m_certificate(std::move(certificate))
    , m_key(std::move(key))
{
}

QSslConfiguration TlsIdentity::serverConfiguration() const
{
    QSslConfiguration configuration = baseConfiguration();
    configuration.setLocalCertificate(m_certificate);
    configuration.setPrivateKey(m_key);
    return configuration;
}

QSslConfiguration TlsIdentity::clientConfiguration()
{
    return baseConfiguration();
}

TlsIdentity TlsIdentity::loadOrCreate(const QString &directory)
{
    if (directory.isEmpty() || !QDir().mkpath(directory)) {
        qCWarning(lcNet) << "no directory for the TLS certificate";
        return {};
    }

    const QString certPath = QDir(directory).filePath(kCertFile);
    const QString keyPath = QDir(directory).filePath(kKeyFile);
    QFile certFile(certPath);
    QFile keyFile(keyPath);
    if (certFile.open(QIODevice::ReadOnly) && keyFile.open(QIODevice::ReadOnly)) {
        const QSslCertificate certificate(certFile.readAll(), QSsl::Pem);
        const QSslKey key(keyFile.readAll(), QSsl::Ec, QSsl::Pem, QSsl::PrivateKey);
        if (usable(certificate, key))
            return TlsIdentity(certificate, key);
        qCWarning(lcNet) << "replacing the stored TLS certificate";
    }

    QByteArray certificatePem;
    QByteArray keyPem;
    if (!createSelfSignedIdentity(&certificatePem, &keyPem)) {
        qCWarning(lcNet) << "couldn't create a TLS certificate";
        return {};
    }
    if (!writePrivate(keyPath, keyPem) || !writePrivate(certPath, certificatePem)) {
        qCWarning(lcNet) << "couldn't store the TLS certificate in" << directory;
        return {};
    }

    const QSslCertificate certificate(certificatePem, QSsl::Pem);
    const QSslKey key(keyPem, QSsl::Ec, QSsl::Pem, QSsl::PrivateKey);
    if (!usable(certificate, key))
        return {};
    qCInfo(lcNet) << "TLS certificate" << textFromFingerprint(fingerprintOf(certificate));
    return TlsIdentity(certificate, key);
}

QByteArray TlsIdentity::fingerprintOf(const QSslCertificate &certificate)
{
    if (certificate.isNull())
        return {};
    const QByteArray digest = certificate.digest(QCryptographicHash::Sha256);
    return digest.size() == 32 ? digest : QByteArray();
}

QString TlsIdentity::textFromFingerprint(const QByteArray &fingerprint)
{
    if (fingerprint.size() != 32)
        return {};
    return QString::fromLatin1(fingerprint.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

QByteArray TlsIdentity::fingerprintFromText(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.size() != 43)
        return {};
    const QByteArray raw = QByteArray::fromBase64(trimmed.toLatin1(),
                                                   QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    if (raw.size() != 32 || textFromFingerprint(raw) != trimmed)
        return {};
    return raw;
}

bool TlsIdentity::pinPeer(const QSslSocket *socket, const QByteArray &expected, QByteArray *seen)
{
    const QByteArray fingerprint = socket ? fingerprintOf(socket->peerCertificate()) : QByteArray();
    if (seen)
        *seen = fingerprint;
    if (fingerprint.size() != 32)
        return false;
    return expected.isEmpty() || expected == fingerprint;
}

} // namespace beamr
