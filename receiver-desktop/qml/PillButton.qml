import QtQuick
import QtQuick.Controls.Basic
import Beamr.Receiver

// Text button in three weights: primary (accent), secondary (outlined)
// and danger.
Button {
    id: control

    property string kind: "secondary"

    readonly property color fillColor: kind === "primary" ? Theme.accent
                                     : kind === "danger" ? Theme.danger
                                     : "transparent"

    focusPolicy: Qt.TabFocus
    leftPadding: 18
    rightPadding: 18
    topPadding: 9
    bottomPadding: 9
    font.pixelSize: 14
    font.weight: Font.DemiBold

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.kind === "primary" ? Theme.accentInk
             : control.kind === "danger" ? "white"
             : Theme.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitHeight: 38
        radius: height / 2
        color: control.fillColor
        border.width: control.kind === "secondary" || control.visualFocus ? 1 : 0
        border.color: control.visualFocus ? Theme.accent : Theme.border

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: control.down ? Theme.pressed : control.hovered ? Theme.hover : "transparent"
        }
    }
}
