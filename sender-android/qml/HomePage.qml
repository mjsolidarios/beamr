pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Sender

// The whole app on one screen: the cast, the receivers it goes to, and a
// way to add another.
Flickable {
    id: root

    readonly property bool hasSessions: SenderController.sessions.count > 0
    readonly property bool showCastCard: SenderController.approvedCount > 0
                                         || SenderController.castState !== SenderController.Off
    // Recent receivers not already in the list; tapping one adds it.
    readonly property var availableRecent: {
        SenderController.sessions.count // re-evaluate as sessions come and go
        return SenderController.recentReceivers.filter(r => !SenderController.isConnectedTo(r.address))
    }
    // On first use, scanning the computer's code is the way in; typing an
    // address is the fallback (and the only way where there's no scanner).
    readonly property bool firstUse: !hasSessions && !hasRecent
    readonly property bool addressIsPrimary: firstUse && !SenderController.canScan
    readonly property bool hasRecent: SenderController.canAddReceiver && availableRecent.length > 0
    // Retrying only makes sense for a receiver that isn't back in the list.
    readonly property bool canRetry: {
        SenderController.sessions.count // re-evaluate as sessions come and go
        return SenderController.retryAddress.length > 0 && SenderController.canAddReceiver
               && !SenderController.isConnectedTo(SenderController.retryAddress)
    }

    function addReceiver(address) {
        const error = SenderController.validateAddress(address)
        addressField.error = error
        if (error.length > 0) {
            addressField.forceActiveFocus()
            SenderController.haptic(false)
            return
        }
        Qt.inputMethod.hide()
        addressField.focus = false
        addressField.clear()
        SenderController.addReceiver(address)
    }

    contentWidth: width
    contentHeight: column.implicitHeight + 2 * Theme.gutter
    boundsBehavior: Flickable.StopAtBounds
    clip: true

    ColumnLayout {
        id: column

        x: Theme.gutter
        y: Theme.gutter / 2
        width: root.width - 2 * Theme.gutter
        spacing: 0

        // Empty state: what this app is for and how to start. With saved
        // computers below, one tap is all it takes, so say only that.
        ColumnLayout {
            Layout.fillWidth: true
            visible: !root.hasSessions
            spacing: 8

            Label {
                Layout.fillWidth: true
                Layout.topMargin: 16
                text: qsTr("Cast to your computers")
                color: Theme.text
                font.pixelSize: 30
                font.weight: Font.Light
                wrapMode: Text.Wrap
            }

            Label {
                Layout.fillWidth: true
                text: root.hasRecent
                      ? qsTr("Tap a computer running beamr to show your screen on it.")
                      : SenderController.canScan
                        ? qsTr("Open beamr on a computer and scan the QR code it shows.")
                        : qsTr("Open beamr on a computer and enter the address it shows.")
                color: Theme.textMuted
                font.pixelSize: 15
                lineHeight: 1.15
                wrapMode: Text.Wrap
            }
        }

        PillButton {
            Layout.fillWidth: true
            Layout.topMargin: 24
            visible: root.firstUse && SenderController.canScan
            kind: "primary"
            glyph: "scan-qr-code"
            text: qsTr("Scan QR code")
            onClicked: SenderController.scanQrCode()
        }

        CastCard {
            Layout.fillWidth: true
            Layout.topMargin: 16
            visible: root.showCastCard
        }

        MessageBanner {
            Layout.fillWidth: true
            Layout.topMargin: 16
            visible: SenderController.message.length > 0
            text: SenderController.message
            isError: SenderController.messageIsError
            actionText: root.canRetry ? qsTr("Try again") : ""
            onDismissed: SenderController.clearMessage()
            onActionTriggered: root.addReceiver(SenderController.retryAddress)
        }

        SectionLabel {
            Layout.topMargin: 28
            visible: root.hasSessions
            text: qsTr("Receivers · %1 of %2").arg(SenderController.sessions.count).arg(SenderController.maxReceivers)
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 10
            visible: root.hasSessions
            implicitHeight: sessionColumn.implicitHeight
            radius: Theme.radius
            color: Theme.surface
            border.color: Theme.border
            clip: true

            ColumnLayout {
                id: sessionColumn

                width: parent.width
                spacing: 0

                Repeater {
                    model: SenderController.sessions

                    delegate: SessionRow {
                        Layout.fillWidth: true
                    }
                }
            }
        }

        // Computers this phone has cast to before: the quickest way in, so
        // they come before typing an address.
        SectionLabel {
            Layout.topMargin: 28
            visible: root.hasRecent
            text: qsTr("Recent")
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 10
            visible: root.hasRecent
            implicitHeight: recentColumn.implicitHeight
            radius: Theme.radius
            color: Theme.surface
            border.color: Theme.border
            clip: true

            ColumnLayout {
                id: recentColumn

                width: parent.width
                spacing: 0

                Repeater {
                    model: root.availableRecent

                    delegate: ItemDelegate {
                        id: recentRow

                        required property int index
                        required property var modelData

                        Layout.fillWidth: true
                        leftPadding: 16
                        rightPadding: 12
                        topPadding: 14
                        bottomPadding: 14
                        onClicked: root.addReceiver(modelData.address)
                        // Forgetting is rare; keep it out of the way of connecting.
                        onPressAndHold: {
                            SenderController.haptic(true)
                            forgetMenu.popup(recentRow, recentRow.width - forgetMenu.width - 12, recentRow.height / 2)
                        }
                        Accessible.name: qsTr("Connect to %1").arg(modelData.name)
                        Accessible.description: qsTr("Touch and hold to forget")

                        background: Rectangle {
                            color: recentRow.down ? Theme.pressed : "transparent"

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.leftMargin: 66
                                height: 1
                                visible: recentRow.index > 0
                                color: Theme.border
                            }
                        }

                        contentItem: RowLayout {
                            spacing: 14

                            Rectangle {
                                implicitWidth: 36
                                implicitHeight: 36
                                radius: 18
                                color: Theme.accentSoft

                                Icon {
                                    anchors.centerIn: parent
                                    glyph: "monitor"
                                    size: 18
                                    color: Theme.accent
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1

                                Label {
                                    Layout.fillWidth: true
                                    text: recentRow.modelData.name
                                    color: Theme.text
                                    font.pixelSize: 16
                                    elide: Text.ElideRight
                                }

                                Label {
                                    Layout.fillWidth: true
                                    visible: recentRow.modelData.name !== recentRow.modelData.address
                                    text: recentRow.modelData.address
                                    color: Theme.textMuted
                                    font.pixelSize: 13
                                    font.family: Theme.monoFamily
                                    elide: Text.ElideRight
                                }
                            }

                            // Says what a tap does; the whole row is the target.
                            Label {
                                leftPadding: 14
                                rightPadding: 14
                                topPadding: 7
                                bottomPadding: 7
                                text: qsTr("Connect")
                                color: Theme.accent
                                font.pixelSize: 14
                                font.weight: Font.DemiBold

                                background: Rectangle {
                                    radius: height / 2
                                    color: Theme.accentSoft
                                }
                            }
                        }

                        Menu {
                            id: forgetMenu

                            padding: 6

                            background: Rectangle {
                                implicitWidth: 180
                                radius: Theme.radiusSmall
                                color: Theme.surfaceRaised
                                border.color: Theme.border
                            }

                            MenuItem {
                                id: forgetItem

                                text: qsTr("Forget")
                                onTriggered: SenderController.forgetReceiver(recentRow.modelData.address)

                                contentItem: Text {
                                    leftPadding: 6
                                    text: forgetItem.text
                                    color: Theme.danger
                                    font.pixelSize: 15
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    implicitHeight: Theme.touchTarget
                                    radius: Theme.radiusSmall - 4
                                    color: forgetItem.down || forgetItem.highlighted ? Theme.pressed : "transparent"
                                }
                            }
                        }
                    }
                }
            }
        }

        SectionLabel {
            id: addressLabel

            Layout.topMargin: 28
            text: root.hasSessions ? qsTr("Add another receiver")
                : root.firstUse && SenderController.canScan ? qsTr("Or enter an address")
                : root.hasRecent ? qsTr("Or enter an address")
                : qsTr("Receiver address")
            color: addressField.error.length > 0 ? Theme.danger
                 : addressField.activeFocus ? Theme.accent : Theme.textMuted
        }

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 8
            visible: !SenderController.canAddReceiver
            text: qsTr("That's the most receivers at once. Remove one to add another.")
            color: Theme.textMuted
            font.pixelSize: 14
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 8
            visible: SenderController.canAddReceiver
            spacing: 10

            TextField {
                id: addressField

                property string error

                Layout.fillWidth: true
                // Recent rows already offer the last address; only prefill
                // when the field is the one way in.
                text: root.hasSessions || root.hasRecent ? "" : SenderController.lastAddress
                placeholderText: "192.168.1.20"
                placeholderTextColor: Theme.textFaint
                color: Theme.text
                font.pixelSize: 20
                font.family: Theme.monoFamily
                leftPadding: 18
                rightPadding: clearButton.visible || scanButton.visible ? Theme.touchTarget + 6 : 18
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText | Qt.ImhUrlCharactersOnly
                selectByMouse: true
                onTextEdited: error = ""
                onAccepted: root.addReceiver(text)

                background: Rectangle {
                    implicitHeight: 56
                    radius: Theme.radiusSmall
                    color: Theme.surface
                    border.width: addressField.activeFocus || addressField.error.length > 0 ? 2 : 1
                    border.color: addressField.error.length > 0 ? Theme.danger
                                : addressField.activeFocus ? Theme.accent : Theme.border
                }

                ToolButton {
                    id: clearButton

                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: Theme.touchTarget
                    height: Theme.touchTarget
                    visible: addressField.text.length > 0 && addressField.activeFocus
                    onClicked: {
                        addressField.clear()
                        addressField.error = ""
                    }
                    Accessible.name: qsTr("Clear address")

                    contentItem: Icon {
                        glyph: "x"
                        size: 18
                        color: Theme.textMuted
                    }
                    background: null
                }

                // Scanning fills the address in for you, so it lives in the field.
                ToolButton {
                    id: scanButton

                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: Theme.touchTarget
                    height: Theme.touchTarget
                    visible: SenderController.canScan && !root.firstUse && !clearButton.visible
                    onClicked: SenderController.scanQrCode()
                    Accessible.name: qsTr("Scan QR code")

                    contentItem: Icon {
                        glyph: "scan-qr-code"
                        size: 22
                        color: Theme.accent
                    }
                    background: Rectangle {
                        radius: width / 2
                        color: scanButton.down ? Theme.pressed : "transparent"
                    }
                }
            }

            // When the address isn't the main way in, adding is secondary: a
            // compact button beside the field instead of a full-width one below it.
            PillButton {
                visible: !root.addressIsPrimary
                kind: "secondary"
                text: qsTr("Add")
                enabled: addressField.text.trim().length > 0
                onClicked: root.addReceiver(addressField.text)
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 8
            visible: SenderController.canAddReceiver
            text: addressField.error.length > 0
                  ? addressField.error
                  : qsTr("Port %1 is used unless you add another, like 192.168.1.20:5000.").arg(SenderController.defaultPort)
            color: addressField.error.length > 0 ? Theme.danger : Theme.textFaint
            font.pixelSize: 13
            wrapMode: Text.Wrap
        }

        PillButton {
            Layout.fillWidth: true
            Layout.topMargin: 20
            visible: root.addressIsPrimary
            kind: "primary"
            text: qsTr("Connect")
            enabled: addressField.text.trim().length > 0
            onClicked: root.addReceiver(addressField.text)
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 20
            spacing: 10

            Icon {
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 1
                glyph: "info"
                size: 16
                color: Theme.textFaint
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("Your phone and the computers need to be on the same Wi-Fi network.")
                color: Theme.textFaint
                font.pixelSize: 13
                wrapMode: Text.Wrap
            }
        }
    }
}
