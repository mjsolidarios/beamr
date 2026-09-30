import QtQuick
import QtQuick.Controls.Basic
import Beamr.Sender

// Full-height touch button in three weights: primary (accent), secondary
// (outlined) and danger.
Button {
    id: control

    property string kind: "secondary"
    // Optional icon from icons/, before the label.
    property string glyph

    readonly property color fillColor: kind === "primary" ? Theme.accent
                                     : kind === "danger" ? Theme.danger
                                     : "transparent"

    leftPadding: 24
    rightPadding: 24
    font.pixelSize: 16
    font.weight: Font.DemiBold
    opacity: enabled ? 1 : 0.4
    // Press feedback lands on press-in.
    scale: down ? 0.97 : 1

    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

    readonly property color inkColor: kind === "primary" ? Theme.accentInk
                                    : kind === "danger" ? "white"
                                    : Theme.text

    contentItem: Item {
        implicitWidth: label.implicitWidth + (icon.visible ? icon.width + 10 : 0)
        implicitHeight: label.implicitHeight

        Row {
            anchors.centerIn: parent
            width: Math.min(implicitWidth, parent.width)
            spacing: 10

            Icon {
                id: icon

                anchors.verticalCenter: parent.verticalCenter
                visible: control.glyph.length > 0
                glyph: control.glyph
                size: 20
                color: control.inkColor
            }

            Text {
                id: label

                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(implicitWidth, parent.parent.width - (icon.visible ? icon.width + 10 : 0))
                text: control.text
                font: control.font
                color: control.inkColor
                elide: Text.ElideRight
            }
        }
    }

    background: Rectangle {
        implicitHeight: 52
        radius: height / 2
        color: control.fillColor
        border.width: control.kind === "secondary" ? 1 : 0
        border.color: Theme.border

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: control.down ? Theme.pressed : "transparent"
        }
    }
}
