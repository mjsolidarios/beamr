import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtMultimedia
import Beamr.Receiver

// One screen's picture: the phone's video, or the demo pattern.
Item {
    id: root

    required property CastScreen screen
    property bool showRecordingBadge
    // Fill the area, cropping the edges, instead of showing it all.
    property bool fill
    // Where the screen's video goes; only one view can show it at a time
    // (the grid, or the window it's popped out into).
    property bool active: true

    function attach() {
        if (active)
            screen.videoSink = videoOutput.videoSink
    }

    onActiveChanged: attach()

    signal screenshotSaved(string path)
    signal screenshotFailed()

    // Captures only the video layer, never the overlays on top of it.
    function capture() {
        const path = ReceiverController.nextScreenshotPath()
        videoLayer.grabToImage(function(result) {
            if (result.saveToFile(path))
                root.screenshotSaved(path)
            else
                root.screenshotFailed()
        })
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    Item {
        id: videoLayer

        anchors.fill: parent

        TestPattern {
            anchors.fill: parent
            visible: root.screen.demo
            running: visible && root.visible && !root.screen.paused
        }

        VideoOutput {
            id: videoOutput

            anchors.fill: parent
            visible: !root.screen.demo
            fillMode: root.fill ? VideoOutput.PreserveAspectCrop : VideoOutput.PreserveAspectFit
            Component.onCompleted: root.attach()
        }

        Label {
            anchors.centerIn: parent
            visible: !root.screen.demo && !root.screen.hasVideo
            text: qsTr("Waiting for video from %1…").arg(root.screen.deviceName)
            color: Theme.textMuted
            font.pixelSize: 16
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.screen.paused
        color: "#80000000"

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 6

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Paused")
                color: "white"
                font.pixelSize: 30
                font.weight: Font.Light
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: root.screen.recording
                      ? qsTr("The picture is frozen and recording is on hold. Press Space to resume.")
                      : qsTr("The picture is frozen. Press Space to resume.")
                color: "#ccffffff"
                font.pixelSize: 14
            }
        }
    }

    RecordingBadge {
        screen: root.screen
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 16
        visible: root.showRecordingBadge && root.screen.recording
    }
}
