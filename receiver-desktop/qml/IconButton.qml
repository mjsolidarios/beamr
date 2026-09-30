import QtQuick
import QtQuick.Controls.Basic
import Beamr.Receiver

ToolButton {
    id: control

    property string iconName
    property string tip
    property color tint: Theme.text

    focusPolicy: Qt.NoFocus
    padding: 10
    spacing: 8
    font.pixelSize: 14
    display: text.length > 0 ? AbstractButton.TextBesideIcon : AbstractButton.IconOnly

    icon.source: iconName.length > 0 ? Qt.resolvedUrl("icons/" + iconName + ".svg") : ""
    icon.width: 20
    icon.height: 20
    icon.color: enabled ? tint : Theme.textFaint
    palette.buttonText: icon.color

    ToolTip.visible: hovered && tip.length > 0
    ToolTip.text: tip
    ToolTip.delay: 600

    background: Rectangle {
        implicitWidth: 40
        implicitHeight: 40
        radius: Theme.radiusSmall
        color: control.down ? Theme.pressed : (control.hovered || control.checked) ? Theme.hover : "transparent"
    }
}
