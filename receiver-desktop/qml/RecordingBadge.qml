import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Receiver

Rectangle {
    id: root

    required property CastScreen screen

    implicitWidth: row.implicitWidth + 22
    implicitHeight: 30
    radius: height / 2
    color: "#33ff4757"
    border.color: "#66ff4757"

    RowLayout {
        id: row

        anchors.centerIn: parent
        spacing: 7

        Rectangle {
            implicitWidth: 8
            implicitHeight: 8
            radius: 4
            color: Theme.record

            SequentialAnimation on opacity {
                running: root.visible && !root.screen.paused
                loops: Animation.Infinite
                alwaysRunToEnd: true
                NumberAnimation { to: 0.2; duration: 600 }
                NumberAnimation { to: 1; duration: 600 }
            }
        }

        Label {
            text: root.screen.paused
                  ? qsTr("REC paused %1").arg(Theme.formatDuration(root.screen.recordingSeconds))
                  : qsTr("REC %1").arg(Theme.formatDuration(root.screen.recordingSeconds))
            color: Theme.text
            font.pixelSize: 13
            font.family: Theme.monoFamily
        }
    }
}
