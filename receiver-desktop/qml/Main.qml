import QtQuick
import QtQuick.Controls.Basic
import Beamr.Receiver

ApplicationWindow {
    id: window

    readonly property bool casting: ReceiverController.state === ReceiverController.Casting
    readonly property bool fullScreen: visibility === Window.FullScreen
    readonly property bool popupOpen: recordingsDrawer.opened || settingsDialog.opened
    readonly property var screens: ReceiverController.screens
    // The screen filling the window, when one is picked out of several.
    property CastScreen focusedScreen: null
    // Where the pointer last was; keyboard shortcuts act on it.
    property CastScreen pointedScreen: null
    readonly property CastScreen currentScreen: {
        if (focusedScreen)
            return focusedScreen
        if (pointedScreen && pointedScreen.casting)
            return pointedScreen
        return screens.find(s => s.casting) ?? null
    }
    readonly property ScreenTile currentTile: {
        for (let i = 0; i < tiles.count; ++i) {
            const tile = tiles.itemAt(i)
            if (tile && tile.screen === currentScreen)
                return tile
        }
        return null
    }
    property int lastRequestCount: 0

    function toggleFullScreen() {
        if (fullScreen)
            showNormal()
        else
            showFullScreen()
    }

    function toggleFocus(screen) {
        focusedScreen = focusedScreen === screen ? null : screen
    }

    width: 1280
    height: 760
    minimumWidth: 760
    minimumHeight: 540
    // Started at sign-in: stay in the tray until wanted.
    visible: !DesktopIntegration.startHidden
    title: ReceiverController.castingCount > 1 ? qsTr("beamr · %1 phones").arg(ReceiverController.castingCount)
         : casting ? qsTr("beamr · %1").arg(screens.find(s => s.casting)?.deviceName ?? "")
         : "beamr"
    color: Theme.bg

    palette.window: Theme.surface
    palette.windowText: Theme.text
    palette.base: Theme.surfaceRaised
    palette.alternateBase: Theme.surface
    palette.text: Theme.text
    palette.button: Theme.surfaceRaised
    palette.buttonText: Theme.text
    palette.brightText: Theme.text
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.accentInk
    palette.placeholderText: Theme.textFaint
    palette.toolTipBase: Theme.surfaceRaised
    palette.toolTipText: Theme.text
    palette.light: Theme.surfaceRaised
    palette.midlight: Theme.border
    palette.mid: Theme.border
    palette.dark: Theme.border
    palette.shadow: "black"

    onCastingChanged: {
        if (!casting && fullScreen)
            showNormal()
    }

    property bool toldAboutTray: false

    function bringToFront() {
        if (visibility === Window.Hidden || visibility === Window.Minimized)
            showNormal()
        raise()
        requestActivate()
    }

    // Closing keeps beamr reachable from the tray (unless turned off).
    onClosing: close => {
        if (DesktopIntegration.trayAvailable && DesktopIntegration.keepRunning) {
            close.accepted = false
            hide()
            if (!toldAboutTray) {
                toldAboutTray = true
                DesktopIntegration.notify(qsTr("beamr is still running"),
                                          qsTr("Phones can still connect. Open it or quit from the tray icon."))
            }
        } else {
            DesktopIntegration.quit()
        }
    }

    Connections {
        target: DesktopIntegration

        function onShowRequested() {
            window.bringToFront()
        }
    }
    // Back to the grid when there's nothing left to single out.
    onScreensChanged: {
        if (focusedScreen && (screens.length <= 1 || !screens.includes(focusedScreen)))
            focusedScreen = null
        if (pointedScreen && !screens.includes(pointedScreen))
            pointedScreen = null
    }

    TopBar {
        id: topBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        visible: !window.fullScreen
        fullScreen: window.fullScreen
        onRecordingsRequested: recordingsDrawer.open()
        onSettingsRequested: settingsDialog.open()
        onFullScreenRequested: window.toggleFullScreen()
        onAddScreenRequested: ReceiverController.addScreen()
    }

    Item {
        id: stage

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.top: topBar.visible ? topBar.bottom : parent.top

        // One screen fills the stage; more share it in a grid, two side by
        // side, three or four two by two.
        readonly property int count: window.screens.length
        readonly property bool single: count === 1 || window.focusedScreen !== null
        readonly property int columns: count <= 1 ? 1 : count === 2 && width < height ? 1 : 2
        readonly property int rows: Math.ceil(count / columns)
        readonly property real gap: 12
        readonly property real cellWidth: (width - gap * (columns + 1)) / columns
        readonly property real cellHeight: (height - gap * (rows + 1)) / rows

        Repeater {
            id: tiles

            model: window.screens

            delegate: ScreenTile {
                id: tile

                required property CastScreen modelData
                required property int index

                readonly property int column: index % stage.columns
                readonly property int row: Math.floor(index / stage.columns)

                screen: modelData
                single: stage.single
                focused: window.focusedScreen === modelData
                visible: window.focusedScreen === null || focused
                removable: stage.count > 1
                fullScreen: window.fullScreen
                x: stage.single ? 0 : stage.gap + column * (stage.cellWidth + stage.gap)
                y: stage.single ? 0 : stage.gap + row * (stage.cellHeight + stage.gap)
                width: stage.single ? stage.width : stage.cellWidth
                height: stage.single ? stage.height : stage.cellHeight

                onCopied: toast.show(qsTr("Address copied"))
                onRemoveRequested: ReceiverController.removeScreen(modelData)
                onFocusRequested: window.toggleFocus(modelData)
                onFullScreenRequested: window.toggleFullScreen()
                onActivated: window.pointedScreen = modelData
                onScreenshotSaved: toast.show(qsTr("Screenshot saved"), qsTr("Show"), ReceiverController.screenshotsFolder)
                onScreenshotFailed: toast.show(qsTr("Couldn't save the screenshot"))
            }
        }

        // Fills the grid's empty cell, where another screen would go.
        AbstractButton {
            id: addTile

            readonly property int index: stage.count

            x: stage.gap + (index % stage.columns) * (stage.cellWidth + stage.gap)
            y: stage.gap + Math.floor(index / stage.columns) * (stage.cellHeight + stage.gap)
            width: stage.cellWidth
            height: stage.cellHeight
            visible: !stage.single && stage.count % stage.columns !== 0 && ReceiverController.canAddScreen
            hoverEnabled: true
            onClicked: ReceiverController.addScreen()
            Accessible.name: qsTr("Add screen")

            background: Rectangle {
                radius: Theme.radius
                color: addTile.down ? Theme.pressed : addTile.hovered ? Theme.hover : "transparent"
                border.color: Theme.border
            }

            contentItem: Item {
                Column {
                    anchors.centerIn: parent
                    spacing: 8

                    Icon {
                        anchors.horizontalCenter: parent.horizontalCenter
                        glyph: "plus"
                        size: 28
                        color: Theme.text
                        opacity: addTile.hovered ? 1 : 0.6
                    }

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Add screen")
                        color: Theme.text
                        font.pixelSize: 16
                    }

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Another phone can cast here")
                        color: Theme.textMuted
                        font.pixelSize: 13
                    }
                }
            }
        }
    }

    Column {
        id: requestStack

        anchors.top: stage.top
        anchors.right: parent.right
        anchors.margins: 16
        spacing: 12
        z: 10

        add: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 180 }
            NumberAnimation { property: "x"; from: 48; duration: 220; easing.type: Easing.OutCubic }
        }

        move: Transition {
            NumberAnimation { property: "y"; duration: 180; easing.type: Easing.OutCubic }
        }

        Repeater {
            model: ReceiverController.requests

            delegate: RequestCard {}
        }
    }

    Toast {
        id: toast

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: window.casting ? 96 : 28
        z: 20
    }

    RecordingsDrawer {
        id: recordingsDrawer

        objectName: "recordingsDrawer"
    }

    SettingsDialog {
        id: settingsDialog

        objectName: "settingsDialog"
    }

    Connections {
        target: ReceiverController

        function onNotify(message) {
            toast.show(message)
        }
    }

    // Flash the taskbar entry when a phone asks to cast while we're in the background.
    Connections {
        target: ReceiverController.requests

        function onCountChanged() {
            const count = ReceiverController.requests.count
            if (count > window.lastRequestCount) {
                window.alert(0)
                // Out of sight: say so where it'll be seen.
                if (!window.visible || window.visibility === Window.Minimized || !window.active) {
                    const name = ReceiverController.requests.deviceNameAt(count - 1)
                    DesktopIntegration.notify(qsTr("%1 wants to cast").arg(name),
                                              qsTr("Open beamr to allow or decline."))
                }
            }
            window.lastRequestCount = count
        }
    }

    Shortcut {
        sequence: "F11"
        onActivated: window.toggleFullScreen()
    }

    Shortcut {
        sequence: "Esc"
        enabled: (window.fullScreen || window.focusedScreen) && !window.popupOpen
        onActivated: {
            if (window.fullScreen)
                window.showNormal()
            else
                window.focusedScreen = null
        }
    }

    Shortcut {
        sequence: "Space"
        enabled: window.currentScreen !== null && !window.popupOpen
        onActivated: {
            window.currentScreen.togglePaused()
            ReceiverController.dismissControlHint()
        }
    }

    Shortcut {
        sequence: "R"
        enabled: window.currentScreen !== null && !window.popupOpen
        onActivated: {
            window.currentScreen.toggleRecording()
            ReceiverController.dismissControlHint()
        }
    }

    Shortcut {
        sequence: "M"
        enabled: window.currentScreen !== null && window.currentScreen.hasAudio && !window.popupOpen
        onActivated: {
            ReceiverController.audioScreen = window.currentScreen.audible ? null : window.currentScreen
            ReceiverController.dismissControlHint()
        }
    }

    Shortcut {
        sequence: "S"
        enabled: window.currentTile !== null && !window.popupOpen
        onActivated: {
            window.currentTile.capture()
            ReceiverController.dismissControlHint()
        }
    }

    Shortcut {
        sequences: ["+", "="]
        enabled: window.currentTile !== null && !window.popupOpen
        onActivated: window.currentTile.zoomIn()
    }

    Shortcut {
        sequence: "-"
        enabled: window.currentTile !== null && !window.popupOpen
        onActivated: window.currentTile.zoomOut()
    }

    Shortcut {
        sequence: "Ctrl+0"
        enabled: window.currentTile !== null && !window.popupOpen
        onActivated: window.currentTile.userResetZoom()
    }

    Shortcut {
        sequence: "Ctrl+Shift+D"
        enabled: ReceiverController.demoAvailable
        onActivated: ReceiverController.simulateRequest()
    }
}
