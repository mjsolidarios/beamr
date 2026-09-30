import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Receiver

Rectangle {
    id: root

    property bool fullScreen

    signal recordingsRequested()
    signal settingsRequested()
    signal fullScreenRequested()
    signal addScreenRequested()

    implicitHeight: 72
    height: implicitHeight
    color: Theme.bg

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 28
        anchors.rightMargin: 20
        spacing: 14

        Label {
            text: "beamr"
            color: Theme.text
            font.pixelSize: 24
            font.weight: Font.Bold
            font.letterSpacing: -0.3
        }

        // The idle view shows its own status; this is for while casting.
        StatusPill {
            visible: ReceiverController.state === ReceiverController.Casting
        }

        Item {
            Layout.fillWidth: true
        }

        // Another place for a phone to cast to, with its own QR code.
        IconButton {
            id: addScreenButton

            iconName: "plus"
            text: qsTr("Add screen")
            font.pixelSize: 16
            font.weight: Font.Medium
            leftPadding: 16
            rightPadding: 18
            enabled: ReceiverController.canAddScreen
            tip: ReceiverController.canAddScreen
                 ? qsTr("Show another phone alongside")
                 : qsTr("Up to %1 screens").arg(ReceiverController.maxScreens)
            onClicked: root.addScreenRequested()

            background: Rectangle {
                implicitHeight: 44
                radius: Theme.radius
                color: addScreenButton.down ? Theme.successPressed
                     : addScreenButton.hovered ? Theme.successHover : Theme.successSoft
                border.color: addScreenButton.enabled ? Theme.successBorder : Theme.border
            }
        }

        Rectangle {
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.preferredWidth: 1
            Layout.preferredHeight: 32
            color: Theme.border
        }

        IconButton {
            iconName: "circle-dot"
            tip: qsTr("Recordings")
            onClicked: root.recordingsRequested()

            Rectangle {
                visible: ReceiverController.recordings.count > 0
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 3
                width: Math.max(16, countLabel.implicitWidth + 8)
                height: 16
                radius: 8
                color: Theme.accent

                Label {
                    id: countLabel

                    anchors.centerIn: parent
                    text: ReceiverController.recordings.count
                    color: Theme.accentInk
                    font.pixelSize: 10
                    font.weight: Font.Bold
                }
            }
        }

        IconButton {
            iconName: "settings"
            tip: qsTr("Settings")
            onClicked: root.settingsRequested()
        }

        IconButton {
            iconName: root.fullScreen ? "minimize" : "maximize"
            tip: root.fullScreen ? qsTr("Exit fullscreen (F11)") : qsTr("Fullscreen (F11)")
            onClicked: root.fullScreenRequested()
        }
    }
}
