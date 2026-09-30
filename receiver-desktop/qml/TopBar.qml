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

    implicitHeight: 60
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
        anchors.leftMargin: 20
        anchors.rightMargin: 12
        spacing: 14

        Label {
            text: "beamr"
            color: Theme.text
            font.pixelSize: 21
            font.weight: Font.DemiBold
            font.letterSpacing: 0.5
        }

        StatusPill {}

        Item {
            Layout.fillWidth: true
        }

        // Another place for a phone to cast to, with its own QR code.
        IconButton {
            iconName: "plus"
            text: qsTr("Add screen")
            enabled: ReceiverController.canAddScreen
            tip: ReceiverController.canAddScreen
                 ? qsTr("Show another phone alongside")
                 : qsTr("Up to %1 screens").arg(ReceiverController.maxScreens)
            onClicked: root.addScreenRequested()
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.preferredHeight: 24
            color: Theme.border
        }

        IconButton {
            iconName: "film"
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
            iconName: "sliders"
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
