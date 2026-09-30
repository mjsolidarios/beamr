#pragma once

#include <QByteArray>
#include <QJsonObject>

#include <optional>

class QIODevice;

// Control channel between sender and receiver: newline-delimited JSON
// objects over TCP on kDefaultControlPort.
//
//   sender   -> receiver  {"type":"hello","version":1,"deviceId":..,"name":..,"model":..,"screen":..}
//   receiver -> sender    {"type":"welcome","name":<receiver name>}     right after hello
//   receiver -> sender    {"type":"answer","accepted":true|false,"reason":..,"streamToken":..}
//   either   -> other     {"type":"bye"}, then the connection closes
//
// "screen" is optional: the id from a receiver's QR code, naming which of
// its screens to show the cast on. Without it the receiver picks a free one.
//
// A rejected answer carries a reason ("declined", "expired" or
// "unsupported") and the receiver closes the connection after it. An
// accepted one carries a token for the media stream.
//
// Media stream: a second TCP connection to the same port. The sender opens
// it with one line, {"type":"stream","token":..,"codec":"h264"}, then sends
// frames, each a FrameHeader followed by `size` bytes of Annex B H.264.
// Closing it stops the picture but keeps the session.
namespace beamr::protocol {

inline constexpr int kVersion = 1;

// How long the receiver waits for a hello before hanging up.
inline constexpr int kHelloTimeoutMs = 10'000;

inline constexpr char kHello[] = "hello";
inline constexpr char kWelcome[] = "welcome";
inline constexpr char kAnswer[] = "answer";
inline constexpr char kBye[] = "bye";
inline constexpr char kStream[] = "stream";
inline constexpr char kCodecH264[] = "h264";

inline constexpr char kReasonDeclined[] = "declined";
inline constexpr char kReasonExpired[] = "expired";
inline constexpr char kReasonUnsupported[] = "unsupported";

// Frame flags; the same bits as Android's MediaCodec.BUFFER_FLAG_*.
inline constexpr quint8 kFrameKey = 0x1;
inline constexpr quint8 kFrameConfig = 0x2;

// Big endian on the wire.
struct FrameHeader
{
    static constexpr int kSize = 13;
    // Far above any 1080p frame at our bitrates; anything bigger is garbage.
    static constexpr quint32 kMaxFrameBytes = 8 * 1024 * 1024;

    quint32 size = 0;
    quint8 flags = 0;
    qint64 ptsUs = 0;

    QByteArray encode() const;
    static FrameHeader decode(const char *bytes);
};

QByteArray encode(const QJsonObject &message);

// Reads the next complete message from the device. Returns nullopt when no
// full line is buffered yet. Sets `malformed` when the peer sent garbage or
// an oversized line; the caller should drop the connection.
std::optional<QJsonObject> readMessage(QIODevice *device, bool *malformed);

} // namespace beamr::protocol
