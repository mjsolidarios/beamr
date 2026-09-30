pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Receiver

// A waiting screen when it's the only one: how to connect, with its QR code.
Item {
    id: root

    required property CastScreen screen

    signal copied()

    function copyText(text) {
        clipboard.text = text
        clipboard.selectAll()
        clipboard.copy()
        root.copied()
    }

    // QML has no clipboard API; a hidden TextEdit does the job.
    TextEdit {
        id: clipboard

        visible: false
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(520, root.width - 48)
        spacing: 32

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            Label {
                text: qsTr("Ready to receive")
                color: Theme.success
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 1.4
            }

            Label {
                Layout.fillWidth: true
                text: ReceiverController.receiverName
                color: Theme.text
                font.pixelSize: 40
                font.weight: Font.Light
                elide: Text.ElideRight
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 14

            Repeater {
                model: [
                    qsTr("Open beamr on your Android phone."),
                    qsTr("Tap Scan and point the phone at the code below, or enter an address."),
                    ReceiverController.requireApproval
                        ? qsTr("Allow the request that pops up here.")
                        : qsTr("Casting starts right away. Approval is off in Settings.")
                ]

                delegate: RowLayout {
                    id: step

                    required property int index
                    required property string modelData

                    Layout.fillWidth: true
                    spacing: 14

                    Rectangle {
                        implicitWidth: 28
                        implicitHeight: 28
                        radius: 14
                        color: Theme.surface
                        border.color: Theme.border

                        Label {
                            anchors.centerIn: parent
                            text: step.index + 1
                            color: Theme.textMuted
                            font.pixelSize: 13
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: step.modelData
                        color: Theme.text
                        font.pixelSize: 15
                        wrapMode: Text.Wrap
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: connectRow.implicitHeight + 32
            radius: Theme.radius
            color: Theme.surface
            border.color: Theme.border

            RowLayout {
                id: connectRow

                anchors.fill: parent
                anchors.margins: 16
                spacing: 20

                // Dark on white whatever the theme: that's what scanners
                // read best. The tile's margin is the code's quiet zone.
                Rectangle {
                    Layout.alignment: Qt.AlignTop
                    visible: root.screen.connectLink.length > 0
                    implicitWidth: 176
                    implicitHeight: 176
                    radius: Theme.radiusSmall
                    color: "white"

                    QrCode {
                        anchors.fill: parent
                        anchors.margins: 14
                        text: root.screen.connectLink
                        color: "black"
                    }

                    Accessible.role: Accessible.Graphic
                    Accessible.name: qsTr("QR code to connect your phone to %1").arg(ReceiverController.receiverName)
                }

                ColumnLayout {
                    id: addressColumn

                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: 4

                    Label {
                        visible: ReceiverController.addresses.length > 0
                        text: qsTr("Scan with your phone")
                        color: Theme.textMuted
                        font.pixelSize: 12
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: 1.2
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: ReceiverController.addresses.length > 0
                        text: qsTr("In beamr on your phone, tap Scan QR code.")
                        color: Theme.text
                        font.pixelSize: 15
                        wrapMode: Text.Wrap
                    }

                    Label {
                        Layout.topMargin: ReceiverController.addresses.length > 0 ? 20 : 0
                        text: qsTr("Or enter an address")
                        color: Theme.textMuted
                        font.pixelSize: 12
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: 1.2
                    }

                    Repeater {
                        model: ReceiverController.addresses

                        delegate: RowLayout {
                            id: addressRow

                            required property string modelData
                            readonly property string endpoint: modelData + ":" + ReceiverController.port

                            Layout.fillWidth: true

                            Label {
                                Layout.fillWidth: true
                                text: addressRow.endpoint
                                color: Theme.text
                                font.pixelSize: 17
                                font.family: Theme.monoFamily
                                elide: Text.ElideRight
                            }

                            IconButton {
                                iconName: "copy"
                                tip: qsTr("Copy address")
                                tint: Theme.textMuted
                                onClicked: root.copyText(addressRow.endpoint)
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                        visible: ReceiverController.addresses.length === 0
                        text: qsTr("No network connection. Connect this computer to the same Wi-Fi as your phone.")
                        color: Theme.warning
                        font.pixelSize: 14
                        wrapMode: Text.Wrap
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: ReceiverController.demoAvailable
            text: qsTr("Developer build: press Ctrl+Shift+D to simulate a phone asking to cast.")
            color: Theme.textFaint
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }
    }
}
