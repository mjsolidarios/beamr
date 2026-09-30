pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Receiver

Dialog {
    id: root

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(520, parent.width - 48)
    height: Math.min(implicitHeight, parent.height - 48)
    modal: true
    title: qsTr("Settings")
    padding: 24
    topPadding: 8

    onAboutToShow: nameField.text = ReceiverController.receiverName

    header: Label {
        text: root.title
        color: Theme.text
        font.pixelSize: 20
        font.weight: Font.DemiBold
        leftPadding: 24
        topPadding: 22
        bottomPadding: 10
    }

    background: Rectangle {
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border
    }

    component SectionLabel: Label {
        color: Theme.textMuted
        font.pixelSize: 12
        font.capitalization: Font.AllUppercase
        font.letterSpacing: 1.2
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 22

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                SectionLabel {
                    text: qsTr("Receiver name")
                }

                TextField {
                    id: nameField

                    Layout.fillWidth: true
                    placeholderText: qsTr("Shown on your phone")
                    maximumLength: 40
                    font.pixelSize: 15
                    leftPadding: 12
                    onEditingFinished: ReceiverController.receiverName = text

                    background: Rectangle {
                        implicitHeight: 40
                        radius: Theme.radiusSmall
                        color: Theme.bg
                        border.color: nameField.activeFocus ? Theme.accent : Theme.border
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        text: qsTr("Ask before a device can cast")
                        color: Theme.text
                        font.pixelSize: 15
                    }

                    Label {
                        Layout.fillWidth: true
                        text: ReceiverController.requireApproval
                              ? qsTr("New devices need your OK. Devices you always allow connect straight away.")
                              : qsTr("Any phone on this network that finds this receiver can cast right away.")
                        color: ReceiverController.requireApproval ? Theme.textMuted : Theme.warning
                        font.pixelSize: 13
                        wrapMode: Text.Wrap
                    }
                }

                Switch {
                    checked: ReceiverController.requireApproval
                    palette.dark: Theme.accent
                    palette.window: Theme.text
                    focusPolicy: Qt.TabFocus
                    onToggled: ReceiverController.requireApproval = checked
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                SectionLabel {
                    text: qsTr("Always-allowed devices")
                }

                Label {
                    Layout.fillWidth: true
                    visible: ReceiverController.trustedDevices.length === 0
                    text: qsTr("None yet. Tick “Always allow this device” when you approve a phone.")
                    color: Theme.textFaint
                    font.pixelSize: 13
                    wrapMode: Text.Wrap
                }

                Repeater {
                    model: ReceiverController.trustedDevices

                    delegate: RowLayout {
                        id: trustedRow

                        required property var modelData

                        Layout.fillWidth: true

                        Image {
                            source: Qt.resolvedUrl("icons/smartphone.svg")
                            sourceSize: Qt.size(18, 18)
                            opacity: 0.6
                        }

                        Label {
                            Layout.fillWidth: true
                            text: trustedRow.modelData.name
                            color: Theme.text
                            font.pixelSize: 14
                            elide: Text.ElideRight
                        }

                        PillButton {
                            text: qsTr("Forget")
                            onClicked: ReceiverController.forgetDevice(trustedRow.modelData.deviceId)
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                SectionLabel {
                    text: qsTr("Storage")
                }

                Repeater {
                    model: [
                        { label: qsTr("Recordings"), folder: ReceiverController.recordingsFolder },
                        { label: qsTr("Screenshots"), folder: ReceiverController.screenshotsFolder }
                    ]

                    delegate: RowLayout {
                        id: folderRow

                        required property var modelData

                        Layout.fillWidth: true

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            Label {
                                text: folderRow.modelData.label
                                color: Theme.text
                                font.pixelSize: 14
                            }

                            Label {
                                Layout.fillWidth: true
                                text: folderRow.modelData.folder.toString().replace("file://", "")
                                color: Theme.textMuted
                                font.pixelSize: 12
                                elide: Text.ElideMiddle
                            }
                        }

                        IconButton {
                            iconName: "folder"
                            tip: qsTr("Open folder")
                            tint: Theme.textMuted
                            onClicked: Qt.openUrlExternally(folderRow.modelData.folder)
                        }
                    }
                }
            }

            // Qt's LGPL asks apps to say they use Qt and where its license is.
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6

                SectionLabel {
                    text: qsTr("About")
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("beamr %1 is free software under the MIT license.").arg(ReceiverController.appVersion)
                    color: Theme.text
                    font.pixelSize: 14
                    wrapMode: Text.Wrap
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Built with <a href=\"https://www.qt.io\">Qt</a> %1, used under the "
                               + "<a href=\"https://www.gnu.org/licenses/lgpl-3.0.html\">GNU LGPL v3</a>, "
                               + "and developed in Qt Creator. Qt's source is at "
                               + "<a href=\"https://code.qt.io\">code.qt.io</a>. Video and sound decoding "
                               + "use FFmpeg (LGPL v2.1). "
                               + "<a href=\"https://github.com/mjsolidarios/beamr/blob/main/THIRD_PARTY_NOTICES.md\">"
                               + "All licenses</a>").arg(ReceiverController.qtVersion)
                    textFormat: Text.StyledText
                    linkColor: Theme.accent
                    color: Theme.textMuted
                    font.pixelSize: 13
                    wrapMode: Text.Wrap
                    onLinkActivated: link => Qt.openUrlExternally(link)

                    HoverHandler {
                        cursorShape: parent.hoveredLink.length > 0 ? Qt.PointingHandCursor : Qt.ArrowCursor
                    }
                }
            }
        }
    }

    footer: DialogButtonBox {
        alignment: Qt.AlignRight
        padding: 16

        background: Item {}

        PillButton {
            kind: "primary"
            text: qsTr("Done")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }
}
