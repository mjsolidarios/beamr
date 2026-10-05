import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window
import Beamr.Receiver

// One screen in the window: how to connect while it waits, the phone's
// picture and its tools once someone casts. When it's the only screen it
// runs edge to edge. Zoom belongs to the tile, so the grid and a popped-out
// window of this same screen show the same region.
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
    // 1 is the whole picture. focusX/focusY are the fraction kept centred.
    property real zoom: 1
    property real focusX: 0.5
    property real focusY: 0.5
    // One line of shortcuts, on the first cast, until a control is used.
    // Only the first screen says it, so a full grid isn't a row of hints.
    readonly property bool showControlHint: ReceiverController.controlHintPending
                                             && (screen.number === 1 || single)

    signal copied()
    signal removeRequested()
    signal focusRequested()
    signal fullScreenRequested()
    signal screenshotSaved(string path)
    signal screenshotFailed()
    // The pointer is here: keyboard shortcuts act on this screen.
    signal activated()

    function clampZoom(value) {
        return Math.min(4, Math.max(1, value))
    }

    // The centred fraction has to stay on screen: at 2× it lives in [0.25, 0.75].
    function clampFocus() {
        const insetX = 0.5 / zoom
        const insetY = 0.5 / zoom
        focusX = Math.min(1 - insetX, Math.max(insetX, focusX))
        focusY = Math.min(1 - insetY, Math.max(insetY, focusY))
    }

    function zoomBy(factor) {
        zoom = clampZoom(zoom * factor)
        clampFocus()
    }

    // Keep the picture point under the cursor fixed while the scale changes.
    function zoomAt(viewX, viewY, viewW, viewH, factor) {
        if (viewW <= 0 || viewH <= 0 || factor <= 0)
            return
        const contentX = (viewX - viewW / 2) / (viewW * zoom) + focusX
        const contentY = (viewY - viewH / 2) / (viewH * zoom) + focusY
        const next = clampZoom(zoom * factor)
        if (next === zoom)
            return
        zoom = next
        focusX = contentX - (viewX - viewW / 2) / (viewW * zoom)
        focusY = contentY - (viewY - viewH / 2) / (viewH * zoom)
        clampFocus()
    }

    function panBy(dx, dy, viewW, viewH) {
        if (zoom <= 1.01 || viewW <= 0 || viewH <= 0)
            return
        focusX -= dx / (viewW * zoom)
        focusY -= dy / (viewH * zoom)
        clampFocus()
    }

    function zoomIn() {
        zoomBy(1.25)
        useControl()
    }

    function zoomOut() {
        zoomBy(1 / 1.25)
        useControl()
    }

    function resetZoom() {
        zoom = 1
        focusX = 0.5
        focusY = 0.5
    }

    function userResetZoom() {
        resetZoom()
        useControl()
    }

    function useControl() {
        ReceiverController.dismissControlHint()
    }

    // From wherever the picture is showing.
    function capture() {
        if (root.poppedOut && popOut.item)
            popOut.item.capture()
        else
            castView.capture()
    }

    function wakeControls() {
        controlsVisible = true
        // Leave the bar up until the person actually uses one.
        if (ReceiverController.controlHintPending)
            hideControlsTimer.stop()
        else
            hideControlsTimer.restart()
        root.activated()
    }

    function handleZoom(factor, viewX, viewY, viewW, viewH) {
        const before = zoom
        zoomAt(viewX, viewY, viewW, viewH, factor)
        if (zoom !== before)
            useControl()
    }

    Connections {
        target: root.screen

        function onCastingChanged() {
            if (root.screen.casting)
                root.wakeControls()
            else {
                root.poppedOut = false // the picture comes back into the grid
                root.resetZoom()
            }
        }
    }

    Connections {
        target: ReceiverController

        function onControlHintPendingChanged() {
            if (!ReceiverController.controlHintPending && root.controlsVisible && root.screen.casting)
                hideControlsTimer.restart()
        }
    }

    onPoppedOutChanged: {
        if (!poppedOut && screen.casting)
            wakeControls()
    }

    component ShortcutHint: Rectangle {
        radius: height / 2
        color: Theme.overlayBar
        border.color: Theme.border
        implicitWidth: hintText.implicitWidth + 28
        implicitHeight: hintText.implicitHeight + 12

        Label {
            id: hintText

            anchors.centerIn: parent
            text: qsTr("Space pauses  ·  R records  ·  M sound")
            color: Theme.text
            font.pixelSize: 13
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
            zoom: root.zoom
            focusX: root.focusX
            focusY: root.focusY
            blankCursor: root.fullScreen && !root.controlsVisible
            onScreenshotSaved: path => root.screenshotSaved(path)
            onScreenshotFailed: root.screenshotFailed()
            onZoomRequested: (factor, viewX, viewY, viewW, viewH) =>
                             root.handleZoom(factor, viewX, viewY, viewW, viewH)
            onPanRequested: (dx, dy, viewW, viewH) => root.panBy(dx, dy, viewW, viewH)
            onDoubleClicked: root.single ? root.fullScreenRequested() : root.focusRequested()
            onPointerActivity: root.wakeControls()
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

            property bool controlsVisible: true

            function capture() {
                popView.capture()
            }

            function wakeControls() {
                controlsVisible = true
                if (ReceiverController.controlHintPending)
                    popHideTimer.stop()
                else
                    popHideTimer.restart()
            }

            width: 960
            height: 600
            minimumWidth: 320
            minimumHeight: 240
            visible: true
            color: "black"
            title: qsTr("beamr · %1").arg(root.screen.deviceName)
            onClosing: root.poppedOut = false
            Component.onCompleted: {
                if (!ReceiverController.controlHintPending)
                    popHideTimer.restart()
            }

            CastView {
                id: popView

                anchors.fill: parent
                screen: root.screen
                showRecordingBadge: true
                fill: root.fill
                zoom: root.zoom
                focusX: root.focusX
                focusY: root.focusY
                blankCursor: popWindow.visibility === Window.FullScreen && !popWindow.controlsVisible
                onScreenshotSaved: path => root.screenshotSaved(path)
                onScreenshotFailed: root.screenshotFailed()
                onZoomRequested: (factor, viewX, viewY, viewW, viewH) =>
                                 root.handleZoom(factor, viewX, viewY, viewW, viewH)
                onPanRequested: (dx, dy, viewW, viewH) => root.panBy(dx, dy, viewW, viewH)
                onDoubleClicked: popWindow.visibility === Window.FullScreen ? popWindow.showNormal()
                                                                            : popWindow.showFullScreen()
                onPointerActivity: popWindow.wakeControls()
            }

            ShortcutHint {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: popBar.top
                anchors.bottomMargin: 8
                visible: root.showControlHint && popBar.shown
                opacity: popBar.opacity
            }

            ControlBar {
                id: popBar

                readonly property bool shown: popWindow.controlsVisible || hovered || root.screen.paused

                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 24
                enabled: shown
                opacity: shown ? 1 : 0
                screen: root.screen
                fullScreen: popWindow.visibility === Window.FullScreen
                multiScreen: !root.single
                focused: root.focused
                fill: root.fill
                poppedOut: true
                zoom: root.zoom
                onScreenshotRequested: popWindow.capture()
                onFullScreenRequested: popWindow.visibility === Window.FullScreen ? popWindow.showNormal()
                                                                                  : popWindow.showFullScreen()
                onFocusRequested: root.focusRequested()
                onFillToggled: root.fill = !root.fill
                onPopOutRequested: root.poppedOut = false
                onZoomInRequested: root.zoomIn()
                onZoomOutRequested: root.zoomOut()
                onZoomResetRequested: root.userResetZoom()
                onUsed: root.useControl()

                Behavior on opacity {
                    NumberAnimation { duration: 200 }
                }
            }

            Timer {
                id: popHideTimer

                interval: 2500
                onTriggered: popWindow.controlsVisible = false
            }

            Connections {
                target: ReceiverController

                function onControlHintPendingChanged() {
                    if (!ReceiverController.controlHintPending && popWindow.controlsVisible)
                        popHideTimer.restart()
                }
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

            Shortcut {
                sequences: ["+", "="]
                onActivated: root.zoomIn()
            }

            Shortcut {
                sequence: "-"
                onActivated: root.zoomOut()
            }

            Shortcut {
                sequence: "Ctrl+0"
                onActivated: root.userResetZoom()
            }

            Shortcut {
                sequence: "Space"
                onActivated: {
                    root.screen.togglePaused()
                    root.useControl()
                }
            }

            Shortcut {
                sequence: "R"
                onActivated: {
                    root.screen.toggleRecording()
                    root.useControl()
                }
            }

            Shortcut {
                sequence: "M"
                enabled: root.screen.hasAudio
                onActivated: {
                    ReceiverController.audioScreen = root.screen.audible ? null : root.screen
                    root.useControl()
                }
            }

            Shortcut {
                sequence: "S"
                onActivated: {
                    popWindow.capture()
                    root.useControl()
                }
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

        // The picture's own handler sets the same shape while the pointer is
        // over it. This covers the rest of the tile, including fullscreen.
        cursorShape: !root.screen.casting || root.poppedOut ? Qt.ArrowCursor
                     : root.fullScreen && !root.controlsVisible ? Qt.BlankCursor
                     : root.zoom > 1.01 ? Qt.OpenHandCursor
                     : Qt.ArrowCursor
        onPointChanged: {
            if (root.screen.casting)
                root.wakeControls()
        }
    }

    ShortcutHint {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: controlBar.top
        anchors.bottomMargin: 8
        visible: root.showControlHint && root.screen.casting && !root.poppedOut && controlBar.shown
        opacity: controlBar.opacity
    }

    ControlBar {
        id: controlBar

        readonly property bool shown: root.controlsVisible || hovered || root.screen.paused

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.single ? 24 : 12
        visible: root.screen.casting && !root.poppedOut
        enabled: shown
        opacity: shown ? 1 : 0
        screen: root.screen
        fullScreen: root.fullScreen
        multiScreen: !root.single
        focused: root.focused
        fill: root.fill
        poppedOut: root.poppedOut
        zoom: root.zoom
        onScreenshotRequested: root.capture()
        onFullScreenRequested: root.fullScreenRequested()
        onFocusRequested: root.focusRequested()
        onFillToggled: root.fill = !root.fill
        onPopOutRequested: root.poppedOut = !root.poppedOut
        onZoomInRequested: root.zoomIn()
        onZoomOutRequested: root.zoomOut()
        onZoomResetRequested: root.userResetZoom()
        onUsed: root.useControl()

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
