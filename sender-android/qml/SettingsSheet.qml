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

    edge: Qt.BottomEdge
    width: parent.width
    height: content.implicitHeight + 16 + safeBottom
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
            font.pixelSize: 22
            font.weight: Font.DemiBold
        }

        SectionLabel {
            Layout.topMargin: 24
            text: qsTr("Appearance")
        }

        // Segmented control: one choice of three.
        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 10
            implicitHeight: 52
            radius: height / 2
            color: Theme.bg
            border.color: Theme.border

            RowLayout {
                anchors.fill: parent
                anchors.margins: 4
                spacing: 4

                Repeater {
                    model: [
                        { scheme: Qt.Unknown, label: qsTr("System"), glyph: "smartphone" },
                        { scheme: Qt.Light, label: qsTr("Light"), glyph: "sun" },
                        { scheme: Qt.Dark, label: qsTr("Dark"), glyph: "moon" }
                    ]

                    delegate: AbstractButton {
                        id: segment

                        required property var modelData
                        readonly property bool selected: SenderController.colorScheme === modelData.scheme

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        checkable: true
                        checked: selected
                        onClicked: SenderController.colorScheme = modelData.scheme
                        Accessible.name: modelData.label
                        Accessible.role: Accessible.RadioButton

                        background: Rectangle {
                            radius: height / 2
                            color: segment.selected ? Theme.accentSoft
                                 : segment.down ? Theme.pressed : "transparent"
                            border.width: segment.selected ? 1 : 0
                            border.color: Theme.accent

                            Behavior on color { ColorAnimation { duration: 150 } }
                        }

                        contentItem: Item {
                            Row {
                                anchors.centerIn: parent
                                spacing: 8

                                Icon {
                                    anchors.verticalCenter: parent.verticalCenter
                                    glyph: segment.modelData.glyph
                                    size: 18
                                    color: segment.selected ? Theme.accent : Theme.textMuted
                                }

                                Label {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: segment.modelData.label
                                    color: segment.selected ? Theme.accent : Theme.text
                                    font.pixelSize: 15
                                    font.weight: segment.selected ? Font.DemiBold : Font.Normal
                                }
                            }
                        }
                    }
                }
            }
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
                        font.pixelSize: 12
                    }

                    Label {
                        Layout.fillWidth: true
                        text: SenderController.deviceName
                        color: Theme.text
                        font.pixelSize: 15
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

        Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 24
            Layout.bottomMargin: 8
            text: qsTr("beamr %1").arg(root.appVersion)
            color: Theme.textFaint
            font.pixelSize: 12
        }
    }
}
