#include "beamr/log.h"

Q_LOGGING_CATEGORY(lcBeamr, "beamr")
Q_LOGGING_CATEGORY(lcNet, "beamr.net")
Q_LOGGING_CATEGORY(lcCodec, "beamr.codec")

namespace beamr {

void installLogPattern()
{
    qSetMessagePattern(QStringLiteral(
        "%{time hh:mm:ss.zzz} %{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}"
        "%{if-critical}C%{endif}%{if-fatal}F%{endif} [%{category}] %{message}"));
}

} // namespace beamr
