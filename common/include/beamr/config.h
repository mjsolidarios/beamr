#pragma once

#include <cstdint>

namespace beamr {

inline constexpr char kAppName[] = "beamr";
inline constexpr char kVersion[] = BEAMR_VERSION;

// Network. Media (SRT, UDP) and control (TCP) share a port number; the
// protocols don't collide. Discovery uses its own UDP port.
inline constexpr std::uint16_t kDefaultMediaPort = 47700;
inline constexpr std::uint16_t kDefaultControlPort = 47700;
inline constexpr std::uint16_t kDiscoveryPort = 47701;

// SRT receive latency. 80 ms leaves room for Wi-Fi retransmits while keeping
// glass-to-glass under ~150 ms; raise it on congested networks.
inline constexpr int kDefaultSrtLatencyMs = 80;

// Video defaults for the sender's encoder.
inline constexpr int kDefaultWidth = 1920;
inline constexpr int kDefaultHeight = 1080;
inline constexpr int kDefaultFps = 60;
inline constexpr int kDefaultBitrateBps = 8'000'000;
inline constexpr int kMinBitrateBps = 1'000'000;
inline constexpr int kMaxBitrateBps = 20'000'000;
inline constexpr int kKeyframeIntervalSec = 2;

} // namespace beamr
