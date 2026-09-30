#include "streamsender.h"

#include <QJsonObject>
#include <QTcpSocket>

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
}

void StreamSender::open(const QString &sessionId, const QString &host, quint16 port, const QString &token)
{
    drop(sessionId);

    auto *socket = new QTcpSocket(this);
    m_destinations.insert(sessionId, {socket});

    connect(socket, &QTcpSocket::connected, this, [this, socket, sessionId, token] {
        socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
        socket->write(protocol::encode({
            {"type", protocol::kStream},
            {"token", token},
            {"codec", protocol::kCodecH264},
        }));
        qCInfo(lcNet) << "video to" << socket->peerName() << "open";
        // The receiver can only start decoding at a keyframe.
        requestKeyFrame();
        emit opened(sessionId);
    });
    connect(socket, &QTcpSocket::errorOccurred, this, [this, socket, sessionId] {
        const QString error = socket->error() == QAbstractSocket::RemoteHostClosedError
                                  ? tr("the computer closed it")
                                  : socket->errorString();
        qCWarning(lcNet) << "video to" << socket->peerName() << "failed:" << error;
        m_destinations.remove(sessionId);
        socket->disconnect(this);
        socket->deleteLater();
        emit closed(sessionId, error);
    });
    socket->connectToHost(host, port);
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
}

bool StreamSender::drop(const QString &sessionId)
{
    const auto it = m_destinations.constFind(sessionId);
    if (it == m_destinations.cend())
        return false;
    const Destination destination = *it;
    m_destinations.erase(it);

    qCInfo(lcNet) << "video to" << destination.socket->peerName() << "closed; sent" << destination.sentFrames
                  << "frames, skipped" << destination.droppedFrames;
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

    for (Destination &destination : m_destinations)
        send(destination, headerBytes, data, flags);
}

void StreamSender::send(Destination &destination, const QByteArray &header, const QByteArray &data, quint8 flags)
{
    QTcpSocket *socket = destination.socket;
    if (socket->state() != QAbstractSocket::ConnectedState)
        return;

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
            return;
        }
        destination.waitForKeyFrame = false;
    }

    socket->write(header);
    socket->write(data);
    ++destination.sentFrames;
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
