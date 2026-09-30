import QtQuick
import Beamr.Sender

// Onboarding illustrations, drawn with shapes in the app's own colors:
// 0 phone beaming to a monitor, 1 scanning a code, 2 sound, 3 the desktop
// app's grid of phones.
Item {
    id: root

    property int kind
    property bool running

    // Soft glow behind every illustration.
    Repeater {
        model: 3

        delegate: Rectangle {
            required property int index

            anchors.centerIn: parent
            width: root.height * (0.95 - index * 0.22)
            height: width
            radius: width / 2
            color: Theme.accent
            opacity: 0.05 + index * 0.03
        }
    }

    // A phone frame, reused by several illustrations.
    component Phone: Rectangle {
        property real size: 1

        width: 56 * size
        height: 100 * size
        radius: 12 * size
        color: Theme.surface
        border.color: Theme.accent
        border.width: 3

        Rectangle {
            anchors.fill: parent
            anchors.margins: 8 * parent.size
            radius: 5 * parent.size
            color: Theme.accentSoft
        }
    }

    // 0: phone → beam → monitor.
    Item {
        anchors.fill: parent
        visible: root.kind === 0

        Phone {
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 22
            x: parent.width * 0.1
        }

        Rectangle {
            id: monitor

            x: parent.width * 0.42
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -14
            width: parent.width * 0.5
            height: width * 0.66
            radius: 10
            color: Theme.surface
            border.color: Theme.accent
            border.width: 3

            Rectangle {
                anchors.fill: parent
                anchors.margins: 9
                radius: 4
                color: Theme.accentSoft

                // The phone's screen, shown on the monitor.
                Rectangle {
                    anchors.centerIn: parent
                    height: parent.height * 0.8
                    width: height * 0.56
                    radius: 3
                    color: Theme.accent
                    opacity: 0.55
                }
            }

            Rectangle {
                anchors.top: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width * 0.3
                height: 5
                radius: 2
                color: Theme.accent
            }
        }

        // Dots travelling from phone to monitor.
        Repeater {
            model: 3

            delegate: Rectangle {
                id: dot

                required property int index
                property real t: 0

                width: 8
                height: 8
                radius: 4
                color: Theme.accent
                x: root.width * (0.29 + t * 0.12)
                y: root.height * (0.5 - t * 0.1)
                opacity: 1 - Math.abs(t - 0.5) * 1.6

                SequentialAnimation on t {
                    running: root.running && root.kind === 0
                    loops: Animation.Infinite
                    PauseAnimation { duration: dot.index * 300 }
                    NumberAnimation { from: 0; to: 1; duration: 900; easing.type: Easing.InOutSine }
                    PauseAnimation { duration: (2 - dot.index) * 300 }
                }
            }
        }
    }

    // 1: a code inside scanner corners, with a sweeping line.
    Item {
        id: scan

        readonly property real side: Math.min(width, height) * 0.62

        anchors.fill: parent
        visible: root.kind === 1

        Rectangle {
            id: code

            anchors.centerIn: parent
            width: scan.side
            height: scan.side
            radius: 14
            color: "white"

            // A stylised code: finder squares and a scatter of modules.
            Repeater {
                model: [[0, 0], [1, 0], [0, 1]]

                delegate: Rectangle {
                    required property var modelData

                    x: 14 + modelData[0] * (code.width - 28 - width)
                    y: 14 + modelData[1] * (code.height - 28 - height)
                    width: code.width * 0.26
                    height: width
                    radius: 4
                    color: "transparent"
                    border.color: "#0f1115"
                    border.width: width * 0.16

                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width * 0.4
                        height: width
                        radius: 2
                        color: "#0f1115"
                    }
                }
            }

            Grid {
                x: code.width * 0.45
                y: code.height * 0.45
                columns: 5
                spacing: code.width * 0.02

                Repeater {
                    model: [1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1]

                    delegate: Rectangle {
                        required property int modelData

                        width: code.width * 0.075
                        height: width
                        radius: 1
                        color: modelData ? "#0f1115" : "transparent"
                    }
                }
            }

            Rectangle {
                id: sweep

                property real t: 0

                x: 6
                width: parent.width - 12
                height: 3
                radius: 1.5
                y: 8 + t * (parent.height - 16)
                color: Theme.accent

                SequentialAnimation on t {
                    running: root.running && root.kind === 1
                    loops: Animation.Infinite
                    NumberAnimation { from: 0; to: 1; duration: 1400; easing.type: Easing.InOutSine }
                    NumberAnimation { from: 1; to: 0; duration: 1400; easing.type: Easing.InOutSine }
                }
            }
        }

        // Scanner corners.
        Repeater {
            model: 4

            delegate: Item {
                required property int index
                readonly property bool onRight: index % 2 === 1
                readonly property bool onBottom: index >= 2

                width: 34
                height: 34
                x: onRight ? code.x + code.width + 12 - width : code.x - 12
                y: onBottom ? code.y + code.height + 12 - height : code.y - 12

                Rectangle {
                    y: parent.onBottom ? parent.height - 4 : 0
                    width: parent.width
                    height: 4
                    radius: 2
                    color: Theme.accent
                }

                Rectangle {
                    x: parent.onRight ? parent.width - 4 : 0
                    width: 4
                    height: parent.height
                    radius: 2
                    color: Theme.accent
                }
            }
        }
    }

    // 2: a speaker sending out rings.
    Item {
        anchors.fill: parent
        visible: root.kind === 2

        Repeater {
            model: 3

            delegate: Rectangle {
                id: ring

                required property int index
                property real t: 0

                anchors.centerIn: parent
                width: root.height * (0.3 + t * 0.55)
                height: width
                radius: width / 2
                color: "transparent"
                border.color: Theme.accent
                border.width: 3
                opacity: 1 - t

                SequentialAnimation on t {
                    running: root.running && root.kind === 2
                    loops: Animation.Infinite
                    PauseAnimation { duration: ring.index * 600 }
                    NumberAnimation { from: 0; to: 1; duration: 1800; easing.type: Easing.OutCubic }
                    PauseAnimation { duration: (2 - ring.index) * 600 }
                }
            }
        }

        Rectangle {
            anchors.centerIn: parent
            width: root.height * 0.36
            height: width
            radius: width / 2
            color: Theme.accent

            Icon {
                anchors.centerIn: parent
                glyph: "volume-2"
                size: parent.width * 0.5
                color: Theme.accentInk
            }
        }
    }

    // 3: the desktop app's window, four phones side by side.
    Rectangle {
        id: desk

        readonly property real cellW: (width - 30) / 2
        readonly property real cellH: (height - 40) / 2

        anchors.centerIn: parent
        visible: root.kind === 3
        width: parent.width * 0.86
        height: width * 0.62
        radius: 12
        color: Theme.surface
        border.color: Theme.accent
        border.width: 3

        Grid {
            x: 10
            y: 18
            columns: 2
            spacing: 10

            Repeater {
                model: 4

                delegate: Rectangle {
                    id: cell

                    required property int index

                    width: desk.cellW
                    height: desk.cellH
                    radius: 5
                    color: Theme.accentSoft
                    border.color: index === 0 ? Theme.accent : "transparent"
                    border.width: 2

                    // Each tile shows a phone's screen; the first one plays sound.
                    Rectangle {
                        anchors.centerIn: parent
                        height: parent.height * 0.78
                        width: height * 0.56
                        radius: 3
                        color: Theme.accent
                        opacity: 0.35 + cell.index * 0.12
                    }
                }
            }
        }

        // Window dots.
        Row {
            x: 10
            y: 6
            spacing: 4

            Repeater {
                model: 3

                delegate: Rectangle {
                    width: 6
                    height: 6
                    radius: 3
                    color: Theme.border
                }
            }
        }
    }
}
