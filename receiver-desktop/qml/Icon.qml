import QtQuick
import QtQuick.Controls.impl
import Beamr.Receiver

// A line icon from icons/, tinted to the theme the way Qt Quick Controls
// tint theirs (the SVGs themselves are white).
IconImage {
    property string glyph
    property int size: 20

    source: glyph.length > 0 ? Qt.resolvedUrl("icons/" + glyph + ".svg") : ""
    sourceSize: Qt.size(size, size)
    color: Theme.text
}
