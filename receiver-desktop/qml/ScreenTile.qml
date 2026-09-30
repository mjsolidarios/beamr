import QtQuick
import QtQuick.Controls.Basic
import Beamr.Receiver

// One screen in the window: how to connect while it waits, the phone's
// picture and its tools once someone casts. When it's the only screen it
// runs edge to edge.
Item {
    id: root

    required property CastScreen screen
    property bool single
    property bool removable
    property bool focused
    property bool fullScreen
    property bool controlsVisible: true

    signal copied()
    signal removeRequested()
    signal focusRequested()
    signal fullScreenRequested()
    signal screenshotSaved(string path)
    signal screenshotFailed()
    // The pointer is here: keyboard shortcuts act on this screen.
    signal activated()

    function capture() {
        castView.capture()
    }

    function wakeControls() {
        controlsVisible = true
        hideControlsTimer.restart()
        root.activated()
    }

    Connections {
        target: root.screen

        function onCastingChanged() {
            if (root.screen.casting)
                root.wakeControls()
        }
    }

    IdleView {
        anchors.fill: parent
        visible: root.single && !root.screen.casting
        screen: root.screen
        onCopied: root.copied()
    }

    WaitingTile {
        anchors.fill: parent
        visible: !root.single && !root.screen.casting
        screen: root.screen
        removable: root.removable
        onRemoveRequested: root.removeRequested()
    }

    Rectangle {
        anchors.fill: parent
        visible: root.screen.casting
        color: "black"
        radius: root.single ? 0 : Theme.radius
        border.color: root.single ? "transparent" : Theme.border

        CastView {
            id: castView

            anchors.fill: parent
            anchors.margins: root.single ? 0 : 1
            screen: root.screen
            showRecordingBadge: true
            onScreenshotSaved: path => root.screenshotSaved(path)
            onScreenshotFailed: root.screenshotFailed()
        }
    }

    // Who's on which screen, when there are several.
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 12
        visible: root.screen.casting && !root.single && !root.screen.recording
        opacity: root.controlsVisible || hover.hovered ? 1 : 0
        implicitWidth: chipLabel.implicitWidth + 24
        implicitHeight: 30
        radius: height / 2
        color: "#cc171a21"
        border.color: Theme.border

        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }

        Label {
            id: chipLabel

            anchors.centerIn: parent
            text: qsTr("%1 · %2").arg(root.screen.number).arg(root.screen.deviceName)
            color: Theme.text
            font.pixelSize: 13
        }
    }

    HoverHandler {
        id: hover
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.screen.casting
        hoverEnabled: true
        cursorShape: root.fullScreen && !root.controlsVisible ? Qt.BlankCursor : Qt.ArrowCursor
        onPositionChanged: root.wakeControls()
        onDoubleClicked: root.single ? root.fullScreenRequested() : root.focusRequested()
    }

    ControlBar {
        id: controlBar

        readonly property bool shown: root.controlsVisible || hovered || root.screen.paused

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.single ? 24 : 12
        visible: root.screen.casting
        enabled: shown
        opacity: shown ? 1 : 0
        screen: root.screen
        fullScreen: root.fullScreen
        multiScreen: !root.single
        focused: root.focused
        onScreenshotRequested: root.capture()
        onFullScreenRequested: root.fullScreenRequested()
        onFocusRequested: root.focusRequested()

        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }
    }

    Timer {
        id: hideControlsTimer

        interval: 2500
        onTriggered: root.controlsVisible = false
    }
}
