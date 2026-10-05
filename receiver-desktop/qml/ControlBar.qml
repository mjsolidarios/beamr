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
    // 1 shows the whole picture. The tile clamps this to 1–4.
    property real zoom: 1
    readonly property alias hovered: hover.hovered

    signal screenshotRequested()
    signal fullScreenRequested()
    signal focusRequested()
    signal fillToggled()
    signal popOutRequested()
    signal zoomInRequested()
    signal zoomOutRequested()
    signal zoomResetRequested()
    // A real control was used, so the first-cast hint can go.
    signal used()

    function noteUsed() {
        used()
    }

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

            focusPolicy: Qt.NoFocus
            iconName: root.screen.paused ? "play" : "pause"
            tip: root.screen.paused ? qsTr("Resume (Space)") : qsTr("Pause (Space)")
            checked: root.screen.paused
            onClicked: {
                root.noteUsed()
                root.screen.togglePaused()
            }
        }

        IconButton {

            focusPolicy: Qt.NoFocus
            iconName: "record"
            tint: root.screen.recording ? Theme.record : Theme.text
            text: root.screen.recording ? Theme.formatDuration(root.screen.recordingSeconds) : ""
            font.family: Theme.monoFamily
            tip: root.screen.recording ? qsTr("Stop recording (R)") : qsTr("Record (R)")
            onClicked: {
                root.noteUsed()
                root.screen.toggleRecording()
            }
        }

        // One phone's sound plays at a time; this picks (or silences) it.
        IconButton {
            focusPolicy: Qt.NoFocus
            visible: root.screen.hasAudio
            iconName: root.screen.audible ? "volume-2" : "volume-x"
            tint: root.screen.audible ? Theme.text : Theme.textMuted
            tip: root.screen.audible ? qsTr("Mute (M)") : qsTr("Play this phone's sound (M)")
            onClicked: {
                root.noteUsed()
                ReceiverController.audioScreen = root.screen.audible ? null : root.screen
            }
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
            onMoved: {
                root.noteUsed()
                root.screen.volume = value
            }
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

            focusPolicy: Qt.NoFocus
            iconName: "camera"
            tip: qsTr("Screenshot (S)")
            onClicked: {
                root.noteUsed()
                root.screenshotRequested()
            }
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: 1
            height: 24
            color: Theme.border
        }

        IconButton {

            focusPolicy: Qt.NoFocus
            iconName: root.fill ? "scan" : "crop"
            tip: root.fill ? qsTr("Show the whole picture") : qsTr("Fill the space (crops the edges)")
            onClicked: {
                root.noteUsed()
                root.fillToggled()
            }
        }

        IconButton {
            focusPolicy: Qt.NoFocus
            iconName: "minus"
            tip: qsTr("Zoom out (-)")
            enabled: root.zoom > 1.01
            onClicked: {
                root.noteUsed()
                root.zoomOutRequested()
            }
        }

        ToolButton {
            id: zoomReset

            focusPolicy: Qt.NoFocus
            text: Math.round(root.zoom * 100) + "%"
            font.family: Theme.monoFamily
            font.pixelSize: 13
            padding: 8
            Accessible.name: qsTr("Reset zoom")
            onClicked: {
                root.noteUsed()
                root.zoomResetRequested()
            }

            ToolTip.visible: hovered
            ToolTip.delay: 600
            ToolTip.text: qsTr("Reset zoom")

            contentItem: Text {
                text: zoomReset.text
                font: zoomReset.font
                color: root.zoom > 1.01 ? Theme.accent : Theme.text
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                implicitWidth: 52
                implicitHeight: 40
                radius: Theme.radiusSmall
                color: zoomReset.down ? Theme.pressed : zoomReset.hovered ? Theme.hover : "transparent"
            }
        }

        IconButton {
            focusPolicy: Qt.NoFocus
            iconName: "plus"
            tip: qsTr("Zoom in (+)")
            enabled: root.zoom < 3.99
            onClicked: {
                root.noteUsed()
                root.zoomInRequested()
            }
        }

        IconButton {

            focusPolicy: Qt.NoFocus
            iconName: "pop-out"
            checked: root.poppedOut
            tip: root.poppedOut ? qsTr("Bring back into this window") : qsTr("Open in its own window")
            onClicked: {
                root.noteUsed()
                root.popOutRequested()
            }
        }

        IconButton {

            focusPolicy: Qt.NoFocus
            visible: root.multiScreen
            iconName: root.focused ? "layout-grid" : "focus"
            tip: root.focused ? qsTr("Show all screens") : qsTr("Show only this screen")
            onClicked: {
                root.noteUsed()
                root.focusRequested()
            }
        }

        IconButton {

            focusPolicy: Qt.NoFocus
            iconName: root.fullScreen ? "minimize" : "maximize"
            tip: root.fullScreen ? qsTr("Exit fullscreen (F11)") : qsTr("Fullscreen (F11)")
            onClicked: {
                root.noteUsed()
                root.fullScreenRequested()
            }
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: 1
            height: 24
            color: Theme.border
        }

        IconButton {

            focusPolicy: Qt.NoFocus
            iconName: "x"
            text: qsTr("Stop")
            tint: Theme.danger
            tip: qsTr("Stop casting")
            onClicked: {
                root.noteUsed()
                root.screen.stopCasting()
            }
        }
    }
}
