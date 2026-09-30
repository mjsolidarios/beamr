pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Beamr.Receiver

Drawer {
    id: root

    property int pendingRow: -1
    property string pendingName

    edge: Qt.RightEdge
    width: Math.min(440, parent.width * 0.9)
    height: parent.height

    background: Rectangle {
        color: Theme.surface

        Rectangle {
            width: 1
            height: parent.height
            color: Theme.border
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 20
            Layout.rightMargin: 12

            Label {
                Layout.fillWidth: true
                text: qsTr("Recordings")
                color: Theme.text
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }

            IconButton {
                iconName: "folder"
                tip: qsTr("Open recordings folder")
                onClicked: Qt.openUrlExternally(ReceiverController.recordingsFolder)
            }

            IconButton {
                iconName: "x"
                tip: qsTr("Close")
                onClicked: root.close()
            }
        }

        ListView {
            id: list

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: ReceiverController.recordings
            spacing: 2
            ScrollBar.vertical: ScrollBar {}

            delegate: ItemDelegate {
                id: row

                required property int index
                required property string fileName
                required property url fileUrl
                required property string sizeText
                required property string dateText

                width: ListView.view.width
                leftPadding: 20
                rightPadding: 12
                topPadding: 10
                bottomPadding: 10
                onDoubleClicked: Qt.openUrlExternally(row.fileUrl)

                background: Rectangle {
                    color: row.hovered ? Theme.hover : "transparent"
                }

                contentItem: RowLayout {
                    spacing: 12

                    Image {
                        source: Qt.resolvedUrl("icons/film.svg")
                        sourceSize: Qt.size(20, 20)
                        opacity: 0.6
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            Layout.fillWidth: true
                            text: row.fileName
                            color: Theme.text
                            font.pixelSize: 14
                            elide: Text.ElideMiddle
                        }

                        Label {
                            text: row.sizeText + "  ·  " + row.dateText
                            color: Theme.textMuted
                            font.pixelSize: 12
                        }
                    }

                    IconButton {
                        iconName: "play"
                        tip: qsTr("Play")
                        tint: Theme.textMuted
                        onClicked: Qt.openUrlExternally(row.fileUrl)
                    }

                    IconButton {
                        iconName: "download"
                        tip: qsTr("Export…")
                        tint: Theme.textMuted
                        enabled: !ReceiverController.exporting
                        onClicked: {
                            root.pendingRow = row.index
                            exportDialog.selectedFile = row.fileUrl
                            exportDialog.open()
                        }
                    }

                    IconButton {
                        iconName: "trash"
                        tip: qsTr("Move to trash")
                        tint: Theme.textMuted
                        onClicked: {
                            root.pendingRow = row.index
                            root.pendingName = row.fileName
                            confirmTrash.open()
                        }
                    }
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                width: parent.width - 64
                visible: list.count === 0
                spacing: 8

                Image {
                    Layout.alignment: Qt.AlignHCenter
                    source: Qt.resolvedUrl("icons/film.svg")
                    sourceSize: Qt.size(36, 36)
                    opacity: 0.25
                }

                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("No recordings yet")
                    color: Theme.text
                    font.pixelSize: 15
                }

                Label {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Press R while a phone is casting to record the stream.")
                    color: Theme.textMuted
                    font.pixelSize: 13
                    wrapMode: Text.Wrap
                }
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.margins: 20
            visible: ReceiverController.exporting
            text: qsTr("Exporting…")
            color: Theme.accent
            font.pixelSize: 13
        }
    }

    FileDialog {
        id: exportDialog

        title: qsTr("Export recording")
        fileMode: FileDialog.SaveFile
        onAccepted: ReceiverController.exportRecording(root.pendingRow, selectedFile)
    }

    Dialog {
        id: confirmTrash

        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(420, parent.width - 48)
        modal: true
        title: qsTr("Move to trash?")
        padding: 24
        topPadding: 4

        header: Label {
            text: confirmTrash.title
            color: Theme.text
            font.pixelSize: 18
            font.weight: Font.DemiBold
            leftPadding: 24
            topPadding: 22
            bottomPadding: 8
        }

        background: Rectangle {
            radius: Theme.radius
            color: Theme.surface
            border.color: Theme.border
        }

        Label {
            width: parent.width
            text: qsTr("“%1” will be moved to the trash. You can restore it from there.").arg(root.pendingName)
            color: Theme.textMuted
            wrapMode: Text.Wrap
        }

        footer: DialogButtonBox {
            alignment: Qt.AlignRight
            spacing: 10
            padding: 16

            background: Item {}

            PillButton {
                text: qsTr("Cancel")
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }

            PillButton {
                kind: "danger"
                text: qsTr("Move to trash")
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
        }

        onAccepted: ReceiverController.trashRecording(root.pendingRow)
    }
}
