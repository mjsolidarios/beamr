import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window
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
    // Crop to fill instead of showing the whole picture.
    property bool fill
    // Showing in a window of its own (e.g. on a projector).
    property bool poppedOut

    signal copied()
    signal removeRequested()
    signal focusRequested()
    signal fullScreenRequested()
    signal screenshotSaved(string path)
    signal screenshotFailed()
    // The pointer is here: keyboard shortcuts act on this screen.
    signal activated()

    // From wherever the picture is showing.
    function capture() {
        if (root.poppedOut && popOut.item)
            popOut.item.capture()
        else
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
            else
                root.poppedOut = false // the picture comes back into the grid
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
            fill: root.fill
            active: !root.poppedOut
            visible: !root.poppedOut
            onScreenshotSaved: path => root.screenshotSaved(path)
            onScreenshotFailed: root.screenshotFailed()
        }

        // While the picture is in its own window.
        ColumnLayout {
            anchors.centerIn: parent
            visible: root.poppedOut
            spacing: 12

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("%1 is showing in its own window").arg(root.screen.deviceName)
                color: Theme.textMuted
                font.pixelSize: 15
            }

            PillButton {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Bring back")
                onClicked: root.poppedOut = false
            }
        }
    }

    Loader {
        id: popOut

        active: root.poppedOut

        sourceComponent: Window {
            id: popWindow

            function capture() {
                popView.capture()
            }

            width: 960
            height: 600
            minimumWidth: 320
            minimumHeight: 240
            visible: true
            color: "black"
            title: qsTr("beamr · %1").arg(root.screen.deviceName)
            onClosing: root.poppedOut = false

            CastView {
                id: popView

                anchors.fill: parent
                screen: root.screen
                showRecordingBadge: true
                fill: root.fill
                onScreenshotSaved: path => root.screenshotSaved(path)
                onScreenshotFailed: root.screenshotFailed()
            }

            MouseArea {
                anchors.fill: parent
                onDoubleClicked: popWindow.visibility === Window.FullScreen ? popWindow.showNormal()
                                                                            : popWindow.showFullScreen()
            }

            Shortcut {
                sequence: "F11"
                onActivated: popWindow.visibility === Window.FullScreen ? popWindow.showNormal()
                                                                        : popWindow.showFullScreen()
            }

            Shortcut {
                sequence: "Esc"
                onActivated: popWindow.visibility === Window.FullScreen ? popWindow.showNormal() : popWindow.close()
            }
        }
    }

    // The phone dropped off; its last picture stays up while it comes back.
    Rectangle {
        anchors.fill: parent
        visible: root.screen.casting && root.screen.reconnecting
        color: "#b3000000"
        radius: root.single ? 0 : Theme.radius

        Column {
            anchors.centerIn: parent
            spacing: 8

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Reconnecting to %1…").arg(root.screen.deviceName)
                color: "white"
                font.pixelSize: 20
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("The phone dropped off the network. Its screen is held for a few seconds.")
                color: "#ccffffff"
                font.pixelSize: 14
            }
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
        color: Theme.overlayBar
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
        fill: root.fill
        poppedOut: root.poppedOut
        onScreenshotRequested: root.capture()
        onFullScreenRequested: root.fullScreenRequested()
        onFocusRequested: root.focusRequested()
        onFillToggled: root.fill = !root.fill
        onPopOutRequested: root.poppedOut = !root.poppedOut

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
