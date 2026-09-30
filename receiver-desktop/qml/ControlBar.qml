import QtQuick
import QtQuick.Controls.Basic
import Beamr.Receiver

// Tools for one screen's cast.
Rectangle {
    id: root

    required property CastScreen screen
    property bool fullScreen
    // With several screens: whether this one fills the window.
    property bool multiScreen
    property bool focused
    property bool fill
    property bool poppedOut
    readonly property alias hovered: hover.hovered

    signal screenshotRequested()
    signal fullScreenRequested()
    signal focusRequested()
    signal fillToggled()
    signal popOutRequested()

    implicitWidth: row.implicitWidth + 16
    implicitHeight: 56
    width: implicitWidth
    height: implicitHeight
    radius: height / 2
    color: Theme.overlayBar
    border.color: Theme.border

    HoverHandler {
        id: hover
    }

    Row {
        id: row

        anchors.centerIn: parent
        spacing: 4

        IconButton {
            iconName: root.screen.paused ? "play" : "pause"
            tip: root.screen.paused ? qsTr("Resume (Space)") : qsTr("Pause (Space)")
            checked: root.screen.paused
            onClicked: root.screen.togglePaused()
        }

        IconButton {
            iconName: "record"
            tint: root.screen.recording ? Theme.record : Theme.text
            text: root.screen.recording ? Theme.formatDuration(root.screen.recordingSeconds) : ""
            font.family: Theme.monoFamily
            tip: root.screen.recording ? qsTr("Stop recording (R)") : qsTr("Record (R)")
            onClicked: root.screen.toggleRecording()
        }

        // One phone's sound plays at a time; this picks (or silences) it.
        IconButton {
            visible: root.screen.hasAudio
            iconName: root.screen.audible ? "volume-2" : "volume-x"
            tint: root.screen.audible ? Theme.text : Theme.textMuted
            tip: root.screen.audible ? qsTr("Mute (M)") : qsTr("Play this phone's sound (M)")
            onClicked: ReceiverController.audioScreen = root.screen.audible ? null : root.screen
        }

        // This phone's volume, while its sound is the one playing.
        Slider {
            id: volumeSlider

            anchors.verticalCenter: parent.verticalCenter
            visible: root.screen.hasAudio && root.screen.audible
            width: 96
            from: 0
            to: 1
            value: root.screen.volume
            focusPolicy: Qt.NoFocus
            onMoved: root.screen.volume = value
            Accessible.name: qsTr("Volume")

            ToolTip.visible: hovered || pressed
            ToolTip.text: qsTr("Volume %1%").arg(Math.round(value * 100))
            ToolTip.delay: 400

            background: Rectangle {
                x: volumeSlider.leftPadding
                y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                width: volumeSlider.availableWidth
                height: 4
                radius: 2
                color: Theme.border

                Rectangle {
                    width: volumeSlider.visualPosition * parent.width
                    height: parent.height
                    radius: 2
                    color: Theme.accent
                }
            }

            handle: Rectangle {
                x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                implicitWidth: 14
                implicitHeight: 14
                radius: 7
                color: volumeSlider.pressed ? Theme.accent : Theme.text
            }
        }

        IconButton {
            iconName: "camera"
            tip: qsTr("Screenshot (S)")
            onClicked: root.screenshotRequested()
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: 1
            height: 24
            color: Theme.border
        }

        IconButton {
            iconName: root.fill ? "scan" : "crop"
            tip: root.fill ? qsTr("Show the whole picture") : qsTr("Fill the space (crops the edges)")
            onClicked: root.fillToggled()
        }

        IconButton {
            iconName: "pop-out"
            checked: root.poppedOut
            tip: root.poppedOut ? qsTr("Bring back into this window") : qsTr("Open in its own window")
            onClicked: root.popOutRequested()
        }

        IconButton {
            visible: root.multiScreen
            iconName: root.focused ? "layout-grid" : "focus"
            tip: root.focused ? qsTr("Show all screens") : qsTr("Show only this screen")
            onClicked: root.focusRequested()
        }

        IconButton {
            iconName: root.fullScreen ? "minimize" : "maximize"
            tip: root.fullScreen ? qsTr("Exit fullscreen (F11)") : qsTr("Fullscreen (F11)")
            onClicked: root.fullScreenRequested()
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: 1
            height: 24
            color: Theme.border
        }

        IconButton {
            iconName: "x"
            text: qsTr("Stop")
            tint: Theme.danger
            tip: qsTr("Stop casting")
            onClicked: root.screen.stopCasting()
        }
    }
}
