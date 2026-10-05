#include <beamr/connectlink.h>
#include <beamr/notices.h>
#include <beamr/protocol.h>
#include <beamr/tlsidentity.h>

#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QHostAddress>
#include <QSslServer>
#include <QSslSocket>
#include <QTemporaryDir>
#include <QTimer>

namespace protocol = beamr::protocol;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    int failed = 0;
    const auto check = [&](bool ok, const char *name) {
        if (ok) {
            qInfo("ok  %s", name);
        } else {
            qWarning("FAIL %s", name);
            ++failed;
        }
    };

    check(QSslSocket::supportsSsl(), "Qt TLS backend");
    if (!QSslSocket::supportsSsl())
        return 1;

    QTemporaryDir dir;
    check(dir.isValid(), "temp dir");
    const beamr::TlsIdentity identity = beamr::TlsIdentity::loadOrCreate(dir.path());
    check(identity.isValid(), "create identity");
    const beamr::TlsIdentity reloaded = beamr::TlsIdentity::loadOrCreate(dir.path());
    check(reloaded.isValid() && reloaded.fingerprint() == identity.fingerprint(), "reload identity");

    const QString text = identity.fingerprintText();
    check(text.size() == 43 && beamr::TlsIdentity::fingerprintFromText(text) == identity.fingerprint(),
          "fingerprint text");
    check(beamr::TlsIdentity::fingerprintFromText(text.left(42)).isEmpty(), "short fingerprint rejected");
    check(beamr::TlsIdentity::fingerprintFromText(text + QLatin1Char('a')).isEmpty(), "long fingerprint rejected");

    beamr::ConnectLink link;
    link.name = QStringLiteral("Office PC");
    link.screen = QStringLiteral("k3f9x2");
    link.pair = QStringLiteral("pairtoken");
    link.key = text;
    link.hosts = {QStringLiteral("192.168.1.20"), QStringLiteral("10.0.0.5")};
    link.port = 47700;
    const beamr::ConnectLink parsed = beamr::ConnectLink::parse(link.toString());
    check(parsed.isValid() && parsed.key == text && parsed.screen == link.screen && parsed.pair == link.pair
              && parsed.hosts == link.hosts && parsed.port == link.port,
          "connect link carries the key");
    if (parsed.key != text)
        qWarning().noquote() << link.toString();

    check(beamr::notices::text().contains(QStringLiteral("AndroidX"))
              && beamr::notices::text().contains(QStringLiteral("libc++"))
              && beamr::notices::text().contains(QStringLiteral("coroutines"))
              && beamr::notices::text().contains(QStringLiteral("OpenSSL"))
              && beamr::notices::text().contains(QStringLiteral("micro-ecc")),
          "bundled notices");
    check(beamr::notices::license(QStringLiteral("apache-2.0")).isEmpty()
              && beamr::notices::license(QStringLiteral("androidx")).contains(QStringLiteral("Apache"))
              && beamr::notices::license(QStringLiteral("coroutines")).contains(QStringLiteral("Apache"))
              && beamr::notices::license(QStringLiteral("libcxx")).contains(QStringLiteral("LLVM Exceptions"))
              && beamr::notices::license(QStringLiteral("openssl")).contains(QStringLiteral("Apache"))
              && beamr::notices::license(QStringLiteral("micro-ecc")).contains(QStringLiteral("Kenneth MacKay"))
              && beamr::notices::licenses().size() >= 10,
          "bundled license texts");

    if (!identity.isValid())
        return 1;

    QSslServer server;
    server.setSslConfiguration(identity.serverConfiguration());
    server.setHandshakeTimeout(protocol::kHelloTimeoutMs);
    QObject::connect(&server, &QSslServer::sslErrors, &server,
                     [](QSslSocket *socket, const QList<QSslError> &) { socket->ignoreSslErrors(); },
                     Qt::DirectConnection);
    check(server.listen(QHostAddress::LocalHost, 0), "listen");

    QSslSocket client;
    client.setSslConfiguration(beamr::TlsIdentity::clientConfiguration());
    client.setPeerVerifyMode(QSslSocket::VerifyNone);
    QObject::connect(&client, &QSslSocket::sslErrors, &client,
                     [&client](const QList<QSslError> &) { client.ignoreSslErrors(); }, Qt::DirectConnection);

    QEventLoop loop;
    QByteArray received;
    QTcpSocket *accepted = nullptr;
    // Same reason as ControlServer: the socket is pending only after it encrypts.
    const auto takeBytes = [&] {
        if (!accepted)
            return;
        received += accepted->readAll();
        const int newline = received.indexOf('\n');
        if (newline >= 0 && received.size() >= newline + 1 + protocol::FrameHeader::kSize + 4)
            loop.quit();
    };
    QObject::connect(&server, &QTcpServer::pendingConnectionAvailable, &server, [&] {
        accepted = server.nextPendingConnection();
        if (!accepted)
            return;
        QObject::connect(accepted, &QIODevice::readyRead, accepted, takeBytes);
        // Application data can already be buffered when the handshake finishes.
        takeBytes();
    });

    bool encrypted = false;
    bool pinned = false;
    QObject::connect(&client, &QSslSocket::encrypted, &client, [&] {
        encrypted = true;
        QByteArray seen;
        pinned = beamr::TlsIdentity::pinPeer(&client, identity.fingerprint(), &seen);
        client.write("hello\n");
        protocol::FrameHeader header;
        header.size = 4;
        header.flags = protocol::kFrameKey;
        header.ptsUs = 42;
        client.write(header.encode());
        client.write("abcd", 4);
    });

    QTimer::singleShot(8000, &loop, &QEventLoop::quit);
    client.connectToHostEncrypted(QStringLiteral("127.0.0.1"), server.serverPort());
    loop.exec();

    check(encrypted && pinned && accepted != nullptr, "handshake and pin");
    const int newline = received.indexOf('\n');
    bool frameOk = false;
    if (newline >= 0 && received.size() >= newline + 1 + protocol::FrameHeader::kSize + 4) {
        const protocol::FrameHeader header = protocol::FrameHeader::decode(received.constData() + newline + 1);
        const QByteArray payload = received.mid(newline + 1 + protocol::FrameHeader::kSize, 4);
        frameOk = received.left(newline) == "hello" && header.size == 4 && header.flags == protocol::kFrameKey
                  && header.ptsUs == 42 && payload == "abcd";
    }
    check(frameOk, "line and frame over TLS");

    QByteArray seen;
    const QByteArray wrong(32, char(0xab));
    check(client.isEncrypted() && !beamr::TlsIdentity::pinPeer(&client, wrong, &seen) && seen == identity.fingerprint(),
          "wrong pin rejected");
    QByteArray trusted;
    check(beamr::TlsIdentity::pinPeer(&client, {}, &trusted) && trusted == identity.fingerprint(),
          "empty pin trusts the certificate it sees");

    QFile certFile(dir.filePath(QStringLiteral("tls-cert.pem")));
    if (certFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        certFile.write("not a certificate\n");
        certFile.close();
    }
    const beamr::TlsIdentity replaced = beamr::TlsIdentity::loadOrCreate(dir.path());
    check(replaced.isValid() && replaced.fingerprint() != identity.fingerprint(), "corrupt certificate is replaced");

    return failed == 0 ? 0 : 1;
}
