import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Receiver

Rectangle {
    id: root

    property url actionUrl

    function show(message, actionText, actionUrl) {
        messageLabel.text = message
        actionButton.text = actionText || ""
        root.actionUrl = actionUrl || ""
        opacity = 1
        hideTimer.restart()
    }

    implicitWidth: row.implicitWidth + 32
    implicitHeight: 44
    width: Math.min(implicitWidth, parent.width - 32)
    height: implicitHeight
    radius: height / 2
    color: Theme.surfaceRaised
    border.color: Theme.border
    opacity: 0
    visible: opacity > 0

    Behavior on opacity {
        NumberAnimation { duration: 180 }
    }

    HoverHandler {
        id: hover

        onHoveredChanged: hovered ? hideTimer.stop() : hideTimer.restart()
    }

    Timer {
        id: hideTimer

        interval: 3500
        onTriggered: root.opacity = 0
    }

    RowLayout {
        id: row

        anchors.centerIn: parent
        spacing: 12

        Label {
            id: messageLabel

            Layout.maximumWidth: root.parent ? root.parent.width - 180 : 400
            color: Theme.text
            font.pixelSize: 14
            elide: Text.ElideRight
        }

        Button {
            id: actionButton

            visible: text.length > 0
            flat: true
            focusPolicy: Qt.NoFocus
            padding: 4
            font.pixelSize: 14
            font.weight: Font.DemiBold
            onClicked: {
                Qt.openUrlExternally(root.actionUrl)
                root.opacity = 0
            }

            contentItem: Text {
                text: actionButton.text
                font: actionButton.font
                color: Theme.accent
            }

            background: Item {}
        }
    }
}
