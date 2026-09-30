pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Receiver

// A waiting screen when it's the only one: what to do, this screen's QR
// code, and the address to type instead.
Item {
    id: root

    required property CastScreen screen
    readonly property bool online: root.screen.connectLink.length > 0
    // Short windows drop the big icon and tighten up, so the card fits.
    readonly property bool compact: height < 820

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

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: Math.max(height, page.implicitHeight + 64)
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ColumnLayout {
            id: page

            anchors.horizontalCenter: parent.horizontalCenter
            y: Math.max(32, (parent.height - implicitHeight) / 2)
            width: Math.min(1010, root.width - 48)
            spacing: 0

            Image {
                Layout.alignment: Qt.AlignHCenter
                visible: !root.compact
                source: Qt.resolvedUrl("icons/monitor.svg")
                sourceSize: Qt.size(64, 64)
                opacity: 0.7
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: root.compact ? 0 : 18
                text: qsTr("Connect your phone")
                color: Theme.text
                font.pixelSize: root.compact ? 34 : 44
                font.weight: Font.Bold
                font.letterSpacing: -0.5
            }

            Label {
                Layout.fillWidth: true
                Layout.topMargin: 6
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Cast your Android screen to %1.").arg(ReceiverController.receiverName)
                color: Theme.textMuted
                font.pixelSize: 20
                elide: Text.ElideRight
            }

            // This computer and whether it can take a cast.
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: root.compact ? 16 : 24
                implicitWidth: chipRow.implicitWidth + 48
                implicitHeight: 52
                radius: Theme.radius
                color: Theme.surface
                border.color: Theme.border

                RowLayout {
                    id: chipRow

                    anchors.centerIn: parent
                    spacing: 16

                    Image {
                        source: Qt.resolvedUrl("icons/monitor.svg")
                        sourceSize: Qt.size(22, 22)
                        opacity: 0.75
                    }

                    Label {
                        Layout.maximumWidth: 260
                        text: ReceiverController.receiverName
                        color: Theme.text
                        font.pixelSize: 16
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        implicitWidth: 1
                        implicitHeight: 26
                        color: Theme.border
                    }

                    Rectangle {
                        implicitWidth: 10
                        implicitHeight: 10
                        radius: 5
                        color: root.online ? Theme.success : Theme.warning
                    }

                    Label {
                        text: root.online ? qsTr("Ready to receive") : qsTr("Not on a network")
                        color: Theme.text
                        font.pixelSize: 15
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 22
                implicitHeight: cardRow.implicitHeight + 2 * cardRow.anchors.margins
                radius: Theme.radius + 4
                color: Theme.surface
                border.color: Theme.border

                RowLayout {
                    id: cardRow

                    anchors.fill: parent
                    anchors.margins: root.compact ? 28 : 44
                    spacing: root.compact ? 32 : 48

                    // Scan side. Dark on white whatever the theme: that's what
                    // scanners read best. The tile's margin is the quiet zone.
                    ColumnLayout {
                        Layout.alignment: Qt.AlignVCenter
                        visible: root.online
                        spacing: 14

                        Rectangle {
                            readonly property real side: Math.min(300, Math.max(200, root.width * 0.2))

                            implicitWidth: side
                            implicitHeight: side
                            radius: Theme.radius
                            color: "white"

                            QrCode {
                                anchors.fill: parent
                                anchors.margins: Math.round(parent.width * 0.07)
                                text: root.screen.connectLink
                                color: "black"
                            }

                            Accessible.role: Accessible.Graphic
                            Accessible.name: qsTr("QR code to connect your phone to %1").arg(ReceiverController.receiverName)
                        }

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Scan to connect")
                            color: Theme.textMuted
                            font.pixelSize: 15
                        }
                    }

                    Rectangle {
                        Layout.fillHeight: true
                        visible: root.online
                        implicitWidth: 1
                        color: Theme.border
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 0

                        Label {
                            text: qsTr("Start casting in seconds")
                            color: Theme.text
                            font.pixelSize: 26
                            font.weight: Font.DemiBold
                        }

                        Repeater {
                            model: [
                                qsTr("Open beamr on your Android phone."),
                                root.online ? qsTr("Tap Scan and scan this QR code.")
                                            : qsTr("Connect this computer to the same Wi-Fi as your phone."),
                                ReceiverController.requireApproval
                                    ? qsTr("Approve the connection on this computer.")
                                    : qsTr("Casting starts right away. Approval is off in Settings.")
                            ]

                            delegate: RowLayout {
                                id: step

                                required property int index
                                required property string modelData

                                Layout.fillWidth: true
                                Layout.topMargin: step.index === 0 ? 22 : 14
                                spacing: 20

                                Rectangle {
                                    implicitWidth: 40
                                    implicitHeight: 40
                                    radius: 20
                                    color: Theme.bg
                                    border.color: Theme.border

                                    Label {
                                        anchors.centerIn: parent
                                        text: step.index + 1
                                        color: Theme.text
                                        font.pixelSize: 16
                                    }
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: step.modelData
                                    color: Theme.text
                                    font.pixelSize: 17
                                    wrapMode: Text.Wrap
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.topMargin: 28
                            implicitHeight: 1
                            color: Theme.border
                            visible: root.online
                        }

                        Label {
                            Layout.topMargin: 26
                            visible: root.online
                            text: qsTr("Or connect with an address")
                            color: Theme.textMuted
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 1.4
                        }

                        Repeater {
                            model: ReceiverController.addresses

                            delegate: AddressBox {
                                required property string modelData

                                Layout.fillWidth: true
                                Layout.topMargin: 12
                                endpoint: modelData + ":" + ReceiverController.port
                                onCopyRequested: root.copyText(endpoint)
                            }
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.topMargin: 18
                horizontalAlignment: Text.AlignHCenter
                visible: ReceiverController.demoAvailable
                text: qsTr("Developer build: press Ctrl+Shift+D to simulate a phone asking to cast.")
                color: Theme.textFaint
                font.pixelSize: 12
                wrapMode: Text.Wrap
            }
        }
    }

    // An address as a read-only field with its copy button attached.
    component AddressBox: Rectangle {
        id: box

        property string endpoint

        signal copyRequested()

        implicitHeight: 60
        radius: Theme.radiusSmall + 2
        color: Theme.bg
        border.color: Theme.border

        RowLayout {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            Label {
                Layout.fillWidth: true
                leftPadding: 20
                text: box.endpoint
                color: Theme.text
                font.pixelSize: 20
                font.family: Theme.monoFamily
                font.letterSpacing: 0.5
                elide: Text.ElideRight
            }

            Image {
                Layout.rightMargin: 16
                source: Qt.resolvedUrl("icons/copy.svg")
                sourceSize: Qt.size(20, 20)
                opacity: 0.6
            }

            Rectangle {
                Layout.fillHeight: true
                implicitWidth: 1
                color: Theme.border
            }

            AbstractButton {
                id: copyButton

                Layout.fillHeight: true
                implicitWidth: 112
                hoverEnabled: true
                onClicked: box.copyRequested()
                Accessible.name: qsTr("Copy %1").arg(box.endpoint)

                background: Rectangle {
                    // Rounds only the outer corners, like the end of the field.
                    color: "transparent"
                    clip: true

                    Rectangle {
                        anchors.fill: parent
                        anchors.leftMargin: -box.radius
                        radius: box.radius
                        color: copyButton.down ? Theme.pressed : copyButton.hovered ? Theme.hover : Theme.surfaceRaised
                    }
                }

                contentItem: Label {
                    text: qsTr("Copy")
                    color: Theme.text
                    font.pixelSize: 17
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
