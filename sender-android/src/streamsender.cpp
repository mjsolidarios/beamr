#include "streamsender.h"

#include <QJsonObject>
#include <QSslSocket>

#include <beamr/tlsidentity.h>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#endif

#include <beamr/log.h>
#include <beamr/protocol.h>

namespace protocol = beamr::protocol;

namespace {

// About a third of a second at 8 Mbit/s. More than this in flight and the
// picture is visibly late; better to skip ahead.
constexpr qint64 kMaxQueuedBytes = 320 * 1024;
constexpr int kKeyFrameRequestIntervalMs = 500;

} // namespace

StreamSender::StreamSender(QObject *parent)
    : QObject(parent)
{
    m_clock.start();
}

void StreamSender::open(const QString &sessionId, const QString &host, quint16 port, const QString &token,
                        bool audio, const QByteArray &fingerprint)
{
    drop(sessionId);

    auto *socket = new QSslSocket(this);
    socket->setSslConfiguration(beamr::TlsIdentity::clientConfiguration());
    socket->setPeerVerifyMode(QSslSocket::VerifyNone);
    Destination destination;
    destination.socket = socket;
    destination.audio = audio;
    m_destinations.insert(sessionId, destination);

    connect(socket, &QSslSocket::sslErrors, socket,
            [socket](const QList<QSslError> &) { socket->ignoreSslErrors(); }, Qt::DirectConnection);
    // connected() is true before the handshake. Frames wait until the pin matches.
    connect(socket, &QSslSocket::encrypted, this, [this, socket, sessionId, token, audio, fingerprint] {
        auto it = m_destinations.find(sessionId);
        if (it == m_destinations.end() || it->socket != socket)
            return;
        QByteArray seen;
        if (!beamr::TlsIdentity::pinPeer(socket, fingerprint, &seen)) {
            fail(sessionId, socket, tr("The computer's security key changed. Scan its QR code again."));
            return;
        }
        it->ready = true;
        socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
        QJsonObject stream{
            {"type", protocol::kStream},
            {"token", token},
            {"codec", protocol::kCodecH264},
        };
        if (audio)
            stream.insert("audio", protocol::kCodecOpus);
        socket->write(protocol::encode(stream));
        qCInfo(lcNet) << "video to" << socket->peerName() << "open";
        // The receiver can only start decoding at a keyframe.
        requestKeyFrame();
        emit opened(sessionId);
    });
    connect(socket, &QSslSocket::errorOccurred, this, [this, socket, sessionId](QAbstractSocket::SocketError error) {
        const auto it = m_destinations.constFind(sessionId);
        if (it == m_destinations.cend() || it->socket != socket)
            return;
        const bool duringHandshake = !it->ready;
        QString message;
        if (duringHandshake
            && (error == QAbstractSocket::SslHandshakeFailedError || error == QAbstractSocket::RemoteHostClosedError))
            message = tr("Couldn't open a secure video connection.");
        else if (error == QAbstractSocket::RemoteHostClosedError)
            message = tr("the computer closed it");
        else
            message = socket->errorString();
        qCWarning(lcNet) << "video to" << socket->peerName() << "failed:" << message;
        fail(sessionId, socket, message);
    });
    socket->connectToHostEncrypted(host, port);
}

void StreamSender::fail(const QString &sessionId, QSslSocket *socket, const QString &error)
{
    const auto it = m_destinations.find(sessionId);
    if (it == m_destinations.end() || it->socket != socket)
        return;
    m_destinations.erase(it);
    socket->disconnect(this);
    socket->abort();
    socket->deleteLater();
    emit closed(sessionId, error);
}

void StreamSender::close(const QString &sessionId)
{
    if (drop(sessionId))
        emit closed(sessionId, {});
}

void StreamSender::closeAll()
{
    const QList<QString> ids = m_destinations.keys();
    for (const QString &id : ids)
        close(id);
    m_dropsInWindow = 0;
    m_dropWindowStart = -1;
    m_lastDropMs = 0;
    clearCongestion();
}

bool StreamSender::drop(const QString &sessionId)
{
    const auto it = m_destinations.constFind(sessionId);
    if (it == m_destinations.cend())
        return false;
    const Destination destination = *it;
    m_destinations.erase(it);

    qCInfo(lcNet) << "video to" << destination.socket->peerName() << "closed; sent" << destination.sentFrames
                  << "frames, skipped" << destination.droppedFrames << "video and" << destination.droppedAudio
                  << "audio";
    destination.socket->disconnect(this);
    destination.socket->disconnectFromHost();
    destination.socket->deleteLater();
    return true;
}

void StreamSender::sendFrame(const QByteArray &data, quint8 flags, qint64 ptsUs)
{
    if (m_destinations.isEmpty())
        return;

    protocol::FrameHeader header;
    header.size = quint32(data.size());
    header.flags = flags;
    header.ptsUs = ptsUs;
    const QByteArray headerBytes = header.encode();

    bool skippedForCongestion = false;
    bool sentAny = false;
    for (Destination &destination : m_destinations) {
        switch (send(destination, headerBytes, data, flags)) {
        case SendResult::Congested:
            skippedForCongestion = true;
            break;
        case SendResult::Sent:
            sentAny = true;
            break;
        case SendResult::Ignored:
            break;
        }
    }
    // One count per frame, however many receivers skipped it.
    if (skippedForCongestion)
        noteDrop();
    else if (sentAny)
        noteSent();
}

void StreamSender::sendAudio(const QByteArray &data, qint64 ptsUs)
{
    protocol::FrameHeader header;
    header.size = quint32(data.size());
    header.flags = protocol::kFrameAudio;
    header.ptsUs = ptsUs;
    const QByteArray headerBytes = header.encode();

    for (Destination &destination : m_destinations) {
        QSslSocket *socket = destination.socket;
        if (!destination.audio || !destination.ready || !socket->isEncrypted())
            continue;
        // A backed-up connection gets video's catch-up; a skipped 20 ms of
        // sound is a click, late sound is worse.
        if (socket->bytesToWrite() > kMaxQueuedBytes) {
            ++destination.droppedAudio;
            continue;
        }
        socket->write(headerBytes);
        socket->write(data);
    }
}

StreamSender::SendResult StreamSender::send(Destination &destination, const QByteArray &header, const QByteArray &data,
                                             quint8 flags)
{
    QSslSocket *socket = destination.socket;
    if (!destination.ready || !socket->isEncrypted())
        return SendResult::Ignored;

    const bool isConfig = flags & protocol::kFrameConfig;
    const bool isKey = flags & protocol::kFrameKey;
    const bool congested = socket->bytesToWrite() > kMaxQueuedBytes;

    // Codec config is tiny and the decoder needs it; never skip it.
    if (!isConfig) {
        if (congested) {
            if (!destination.waitForKeyFrame)
                qCInfo(lcNet) << "network to" << socket->peerName() << "can't keep up; skipping to the next keyframe";
            destination.waitForKeyFrame = true;
        }
        if (destination.waitForKeyFrame && (!isKey || congested)) {
            ++destination.droppedFrames;
            requestKeyFrame();
            // Waiting for the first keyframe is normal. Only a backed-up link counts.
            return congested ? SendResult::Congested : SendResult::Ignored;
        }
        destination.waitForKeyFrame = false;
    }

    socket->write(header);
    socket->write(data);
    ++destination.sentFrames;
    return SendResult::Sent;
}

void StreamSender::noteDrop()
{
    const qint64 now = m_clock.elapsed();
    m_lastDropMs = now;
    if (m_dropWindowStart < 0 || now - m_dropWindowStart > 2000) {
        m_dropWindowStart = now;
        m_dropsInWindow = 0;
    }
    ++m_dropsInWindow;
    // About half a second of video at 60 fps, sustained, not one busy moment.
    if (!m_congested && m_dropsInWindow >= 30) {
        m_congested = true;
        emit congestionChanged(true);
    }
}

void StreamSender::noteSent()
{
    if (!m_congested || m_clock.elapsed() - m_lastDropMs <= 3000)
        return;
    m_dropsInWindow = 0;
    m_dropWindowStart = -1;
    clearCongestion();
}

void StreamSender::clearCongestion()
{
    if (!m_congested)
        return;
    m_congested = false;
    emit congestionChanged(false);
}

void StreamSender::requestKeyFrame()
{
    if (m_sinceKeyFrameRequest.isValid() && m_sinceKeyFrameRequest.elapsed() < kKeyFrameRequestIntervalMs)
        return;
    m_sinceKeyFrameRequest.start();
#ifdef Q_OS_ANDROID
    QJniObject::callStaticMethod<void>("com/beamr/sender/CaptureBridge", "requestKeyFrame");
#endif
}
