import QtQuick
import QtQuick.Controls.impl

// A line icon from icons/, tinted the way Qt Quick Controls tint theirs.
IconImage {
    id: root

    property string glyph
    property int size: 22

    source: glyph.length > 0 ? Qt.resolvedUrl("icons/" + glyph + ".svg") : ""
    sourceSize: Qt.size(size, size)
    color: "white"
}
