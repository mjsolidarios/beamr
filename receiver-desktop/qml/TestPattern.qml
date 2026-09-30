pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import Beamr.Receiver

// Stand-in for video in demo mode. The moving block and the clock make
// pause (a frozen frame) obvious.
Item {
    id: root

    property bool running: true

    clip: true

    Row {
        id: bars

        width: parent.width
        height: parent.height * 0.68

        Repeater {
            model: ["#c0c0c0", "#c0c000", "#00c0c0", "#00c000", "#c000c0", "#c00000", "#0000c0"]

            delegate: Rectangle {
                required property string modelData

                width: bars.width / 7
                height: bars.height
                color: modelData
            }
        }
    }

    Rectangle {
        id: lowerBand

        anchors.top: bars.bottom
        anchors.bottom: parent.bottom
        width: parent.width
        color: "#141414"

        Rectangle {
            id: block

            width: lowerBand.height * 0.5
            height: width
            anchors.verticalCenter: parent.verticalCenter
            color: "white"

            SequentialAnimation on x {
                loops: Animation.Infinite
                paused: !root.running
                NumberAnimation { from: 0; to: lowerBand.width - block.width; duration: 1800; easing.type: Easing.InOutQuad }
                NumberAnimation { from: lowerBand.width - block.width; to: 0; duration: 1800; easing.type: Easing.InOutQuad }
            }
        }

        Label {
            id: clock

            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 16
            color: "white"
            font.pixelSize: Math.max(14, lowerBand.height * 0.16)
            font.family: Theme.monoFamily
        }

        Timer {
            interval: 16
            repeat: true
            running: root.running && root.visible
            triggeredOnStart: true
            onTriggered: clock.text = new Date().toLocaleTimeString(Qt.locale("C"), "hh:mm:ss.zzz")
        }
    }
}
