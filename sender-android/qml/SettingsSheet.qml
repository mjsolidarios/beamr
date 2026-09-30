pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Sender

// Bottom sheet with the few things worth setting: how the app looks and
// what receivers call this phone.
Drawer {
    id: root

    required property string appVersion
    // The overlay spans the whole screen, navigation bar included.
    readonly property real safeBottom: parent ? parent.SafeArea.margins.bottom : 0

    signal renameRequested()
    signal aboutRequested()

    edge: Qt.BottomEdge
    width: parent.width
    // Scrolls rather than running off a short (or landscape) screen.
    height: Math.min(content.implicitHeight + 16 + safeBottom, parent.height * 0.92)
    modal: true
    dim: true

    Overlay.modal: Rectangle { color: Theme.dark ? "#99000000" : "#59000000" }

    background: Rectangle {
        color: Theme.surfaceRaised
        radius: 24

        // Square off the bottom corners; only the top ones show.
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: parent.radius
            color: parent.color
        }
    }

    Flickable {
        anchors.fill: parent
        anchors.bottomMargin: root.safeBottom
        contentWidth: width
        contentHeight: content.implicitHeight + 16
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ColumnLayout {
            id: content

            x: Theme.gutter
            width: parent.width - 2 * Theme.gutter
            spacing: 0

            // Grab handle.
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 10
                implicitWidth: 36
                implicitHeight: 4
                radius: 2
                color: Theme.border
            }

            Label {
                Layout.topMargin: 18
                text: qsTr("Settings")
                color: Theme.text
                font.pixelSize: Theme.sp(22)
                font.weight: Font.DemiBold
            }

            SectionLabel {
                Layout.topMargin: 24
                text: qsTr("Appearance")
            }

            SegmentedControl {
                Layout.fillWidth: true
                Layout.topMargin: 10
                options: [
                    { value: Qt.Unknown, label: qsTr("System"), glyph: "smartphone" },
                    { value: Qt.Light, label: qsTr("Light"), glyph: "sun" },
                    { value: Qt.Dark, label: qsTr("Dark"), glyph: "moon" }
                ]
                value: SenderController.colorScheme
                onActivated: value => SenderController.colorScheme = value
            }

            SectionLabel {
                Layout.topMargin: 28
                text: qsTr("Picture quality")
            }

            SegmentedControl {
                Layout.fillWidth: true
                Layout.topMargin: 10
                options: [
                    { value: SenderController.Smooth, label: qsTr("Smooth") },
                    { value: SenderController.Balanced, label: qsTr("Balanced") },
                    { value: SenderController.DataSaver, label: qsTr("Data saver") }
                ]
                value: SenderController.quality
                onActivated: value => SenderController.quality = value
            }

            Label {
                Layout.fillWidth: true
                Layout.topMargin: 8
                text: {
                    switch (SenderController.quality) {
                    case SenderController.Balanced:
                        return qsTr("Full HD at 30 frames a second. Easier on the battery and busy Wi‑Fi.")
                    case SenderController.DataSaver:
                        return qsTr("720p at 30 frames a second. For weak Wi‑Fi or older phones.")
                    default:
                        return qsTr("Full HD at 60 frames a second. Best for games and video.")
                    }
                }
                color: Theme.textMuted
                font.pixelSize: Theme.sp(13)
                wrapMode: Text.Wrap
            }

            SectionLabel {
                Layout.topMargin: 28
                text: qsTr("Sound")
            }

            SoundSwitch {
                Layout.fillWidth: true
                Layout.topMargin: 6
                Layout.leftMargin: -6
                Layout.rightMargin: -6
            }

            Label {
                Layout.fillWidth: true
                Layout.topMargin: 2
                visible: SenderController.castState === SenderController.On
                text: qsTr("Quality and sound changes apply the next time you start casting.")
                color: Theme.textFaint
                font.pixelSize: Theme.sp(12)
                wrapMode: Text.Wrap
            }

            SectionLabel {
                Layout.topMargin: 28
                text: qsTr("This phone")
            }

            ItemDelegate {
                id: phoneRow

                Layout.fillWidth: true
                Layout.topMargin: 6
                Layout.leftMargin: -12
                Layout.rightMargin: -12
                leftPadding: 12
                rightPadding: 12
                onClicked: {
                    root.close()
                    root.renameRequested()
                }
                Accessible.name: qsTr("Rename this phone. Currently %1").arg(SenderController.deviceName)

                background: Rectangle {
                    radius: Theme.radiusSmall
                    color: phoneRow.down ? Theme.pressed : "transparent"
                }

                contentItem: RowLayout {
                    spacing: 12

                    Icon {
                        glyph: "smartphone"
                        size: 20
                        color: Theme.textMuted
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Label {
                            text: qsTr("Computers see it as")
                            color: Theme.textMuted
                            font.pixelSize: Theme.sp(12)
                        }

                        Label {
                            Layout.fillWidth: true
                            text: SenderController.deviceName
                            color: Theme.text
                            font.pixelSize: Theme.sp(15)
                            elide: Text.ElideRight
                        }
                    }

                    Icon {
                        glyph: "pencil"
                        size: 18
                        color: Theme.textMuted
                    }
                }
            }

            ItemDelegate {
                id: aboutRow

                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.bottomMargin: 8
                Layout.leftMargin: -12
                Layout.rightMargin: -12
                leftPadding: 12
                rightPadding: 12
                onClicked: {
                    root.close()
                    root.aboutRequested()
                }
                Accessible.name: qsTr("About beamr")

                background: Rectangle {
                    radius: Theme.radiusSmall
                    color: aboutRow.down ? Theme.pressed : "transparent"
                }

                contentItem: RowLayout {
                    spacing: 12

                    Icon {
                        glyph: "info"
                        size: 20
                        color: Theme.textMuted
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Label {
                            text: qsTr("About beamr")
                            color: Theme.text
                            font.pixelSize: Theme.sp(15)
                        }

                        Label {
                            text: qsTr("Version %1 · open source").arg(root.appVersion)
                            color: Theme.textMuted
                            font.pixelSize: Theme.sp(12)
                        }
                    }

                    Icon {
                        glyph: "chevron-right"
                        size: 18
                        color: Theme.textFaint
                    }
                }
            }
        }
    }
}
