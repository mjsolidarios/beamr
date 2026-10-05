import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtMultimedia
import Beamr.Receiver

// One screen's picture: the phone's video, or the demo pattern.
// Zoom and pan live here so the grid and a popped-out window both can,
// while the scale itself is owned by the tile (one value per screen).
Item {
    id: root

    required property CastScreen screen
    property bool showRecordingBadge
    // Fill the area, cropping the edges, instead of showing it all.
    property bool fill
    // Where the screen's video goes; only one view can show it at a time
    // (the grid, or the window it's popped out into).
    property bool active: true
    // Magnification and the picture fraction kept at the centre of the view.
    property real zoom: 1
    property real focusX: 0.5
    property real focusY: 0.5
    // Fullscreen hides the pointer once the controls have faded.
    property bool blankCursor

    function attach() {
        if (active)
            screen.videoSink = videoOutput.videoSink
    }

    onActiveChanged: attach()

    signal screenshotSaved(string path)
    signal screenshotFailed()
    // Wheel notch, with the cursor in this view. The tile owns the scale.
    signal zoomRequested(real factor, real viewX, real viewY, real viewW, real viewH)
    signal panRequested(real dx, real dy, real viewW, real viewH)
    signal doubleClicked()
    // Pointer moved, so the controls can come back. Not a control use.
    signal pointerActivity()

    // The visible picture, zoom and all. Overlays stay out of the file.
    function capture() {
        const path = ReceiverController.nextScreenshotPath()
        viewport.grabToImage(function(result) {
            if (result.saveToFile(path))
                root.screenshotSaved(path)
            else
                root.screenshotFailed()
        })
    }

    Item {
        id: viewport

        anchors.fill: parent
        clip: true

        Rectangle {
            anchors.fill: parent
            color: "black"
        }

        Item {
            id: picture

            width: viewport.width * root.zoom
            height: viewport.height * root.zoom
            x: viewport.width / 2 - root.focusX * width
            y: viewport.height / 2 - root.focusY * height

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
        }
    }

    Label {
        anchors.centerIn: parent
        visible: !root.screen.demo && !root.screen.hasVideo
        text: qsTr("Waiting for video from %1…").arg(root.screen.deviceName)
        color: Theme.textMuted
        font.pixelSize: 16
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

    HoverHandler {
        cursorShape: root.blankCursor ? Qt.BlankCursor
                     : root.zoom > 1.01 ? (pan.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
                     : Qt.ArrowCursor
        onPointChanged: root.pointerActivity()
    }

    WheelHandler {
        id: wheel

        // Don't let the wheel fall through to anything scrolling behind the picture.
        blocking: true
        // One notch is 120 units. A notch grows the picture by 12%.
        onWheel: function(event) {
            // A notch is 120 units. Some trackpads report pixels instead.
            let steps = event.angleDelta.y / 120
            if (steps === 0)
                steps = event.pixelDelta.y / 40
            if (steps === 0)
                return
            event.accepted = true
            root.zoomRequested(Math.pow(1.12, steps), event.x, event.y, root.width, root.height)
            root.pointerActivity()
        }
    }

    DragHandler {
        id: pan

        target: null
        enabled: root.zoom > 1.01
        acceptedButtons: Qt.LeftButton
        onTranslationChanged: function(delta) {
            root.panRequested(delta.x, delta.y, root.width, root.height)
            root.pointerActivity()
        }
    }

    TapHandler {
        acceptedButtons: Qt.LeftButton
        gesturePolicy: TapHandler.DragThreshold
        onDoubleTapped: root.doubleClicked()
    }
}
