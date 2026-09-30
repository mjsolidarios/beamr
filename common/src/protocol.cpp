#include "beamr/protocol.h"

#include <QIODevice>
#include <QJsonDocument>
#include <QtEndian>

namespace beamr::protocol {

namespace {

// Control messages are tiny; anything this long without a newline is not a
// beamr peer.
constexpr qint64 kMaxLineBytes = 16 * 1024;

} // namespace

QByteArray FrameHeader::encode() const
{
    QByteArray bytes(kSize, Qt::Uninitialized);
    qToBigEndian<quint32>(size, bytes.data());
    bytes[4] = char(flags);
    qToBigEndian<qint64>(ptsUs, bytes.data() + 5);
    return bytes;
}

FrameHeader FrameHeader::decode(const char *bytes)
{
    FrameHeader header;
    header.size = qFromBigEndian<quint32>(bytes);
    header.flags = quint8(bytes[4]);
    header.ptsUs = qFromBigEndian<qint64>(bytes + 5);
    return header;
}

QByteArray encode(const QJsonObject &message)
{
    return QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n';
}

std::optional<QJsonObject> readMessage(QIODevice *device, bool *malformed)
{
    *malformed = false;
    if (!device->canReadLine()) {
        *malformed = device->bytesAvailable() > kMaxLineBytes;
        return std::nullopt;
    }

    const QByteArray line = device->readLine(kMaxLineBytes + 1);
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(line, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        *malformed = true;
        return std::nullopt;
    }
    return document.object();
}

} // namespace beamr::protocol
