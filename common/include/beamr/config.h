#pragma once

#include <cstdint>

namespace beamr {

inline constexpr char kAppName[] = "beamr";
inline constexpr char kVersion[] = BEAMR_VERSION;

// Control messages and the media stream share this TCP port. Both use TLS.
// Discovery is a UDP broadcast on its own port.
inline constexpr std::uint16_t kDefaultControlPort = 47700;
inline constexpr std::uint16_t kDiscoveryPort = 47701;

// Video defaults for the sender's encoder.
inline constexpr int kDefaultWidth = 1920;
inline constexpr int kDefaultHeight = 1080;
inline constexpr int kDefaultFps = 60;
inline constexpr int kDefaultBitrateBps = 8'000'000;
inline constexpr int kMinBitrateBps = 1'000'000;
inline constexpr int kMaxBitrateBps = 20'000'000;
inline constexpr int kKeyframeIntervalSec = 2;

} // namespace beamr
