#pragma once

#include <QByteArray>
#include <QJsonObject>

#include <optional>

class QIODevice;

// Control channel between sender and receiver: newline-delimited JSON
// objects over TCP on kDefaultControlPort.
//
//   sender   -> receiver  {"type":"hello","version":1,"deviceId":..,"name":..,"model":..,
//                          "screen":..,"pair":..,"resume":..}
//   receiver -> sender    {"type":"welcome","name":<receiver name>,"audio":["opus"]}   right after hello
//   receiver -> sender    {"type":"answer","accepted":true|false,"reason":..,"streamToken":..,"resume":..}
//   receiver -> sender    {"type":"keyframe"}    send a keyframe soon (e.g. a recording starts)
//   either   -> other     {"type":"bye"}, then the connection closes
//
// "screen" is optional: the id from a receiver's QR code, naming which of
// its screens to show the cast on. Without it the receiver picks a free one.
// "pair", also from the QR code, is a one-time code that skips approval.
// "resume" comes back in an accepted answer: if the connection drops, the
// phone reconnects within kResumeGraceMs with it and carries on in the same
// screen without asking again.
//
// Discovery: the phone broadcasts {"type":"discover","version":1} to UDP
// port kDiscoveryPort; each receiver replies to the sender with
// {"type":"receiver","version":1,"name":..,"port":..,"freeScreens":n}. The
// reply's source address is the receiver's address. Replies never carry
// pairing codes: being on the network doesn't skip approval.
//
// A rejected answer carries a reason ("declined", "expired" or
// "unsupported") and the receiver closes the connection after it. An
// accepted one carries a token for the media stream.
//
// Media stream: a second TCP connection to the same port. The sender opens
// it with one line, {"type":"stream","token":..,"codec":"h264","audio":"opus"},
// then sends frames, each a FrameHeader followed by `size` bytes: Annex B
// H.264, or with kFrameAudio set, one Opus packet (48 kHz stereo). Closing
// it stops the picture and sound but keeps the session.
//
// "audio" in welcome lists the audio codecs a receiver plays; a sender only
// sends sound to receivers that list one, and says so with "audio" in its
// stream line. Both are optional, so older apps carry on without sound.
namespace beamr::protocol {

inline constexpr int kVersion = 1;

// How long the receiver waits for a hello before hanging up.
inline constexpr int kHelloTimeoutMs = 10'000;

inline constexpr char kHello[] = "hello";
inline constexpr char kWelcome[] = "welcome";
inline constexpr char kAnswer[] = "answer";
inline constexpr char kBye[] = "bye";
inline constexpr char kKeyFrame[] = "keyframe";
inline constexpr char kDiscover[] = "discover";
inline constexpr char kReceiver[] = "receiver";

// How long a receiver holds a dropped phone's screen for it to come back.
// Phones can take 20 s just to rejoin Wi-Fi after a drop.
inline constexpr int kResumeGraceMs = 30'000;
inline constexpr char kStream[] = "stream";
inline constexpr char kCodecH264[] = "h264";
inline constexpr char kCodecOpus[] = "opus";
inline constexpr int kAudioSampleRate = 48'000;
inline constexpr int kAudioChannels = 2;

inline constexpr char kReasonDeclined[] = "declined";
inline constexpr char kReasonExpired[] = "expired";
inline constexpr char kReasonUnsupported[] = "unsupported";

// Frame flags; the low bits are Android's MediaCodec.BUFFER_FLAG_*.
inline constexpr quint8 kFrameKey = 0x1;
inline constexpr quint8 kFrameConfig = 0x2;
// Not a video frame: an audio packet.
inline constexpr quint8 kFrameAudio = 0x80;

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
