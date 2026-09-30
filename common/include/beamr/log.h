#pragma once

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcBeamr)
Q_DECLARE_LOGGING_CATEGORY(lcNet)
Q_DECLARE_LOGGING_CATEGORY(lcCodec)

namespace beamr {

// Millisecond timestamps on every log line; needed later to line up
// sender and receiver logs when measuring latency.
void installLogPattern();

} // namespace beamr
