import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Receiver

Rectangle {
    id: root

    required property string requestId
    required property string deviceName
    required property string deviceModel
    required property string address
    required property int secondsLeft
    required property string screenId
    readonly property int screenNumber: ReceiverController.screens.length > 1
                                        ? ReceiverController.screenNumber(screenId) : 0
    // Re-read whenever casts start or stop.
    readonly property string replaces: {
        ReceiverController.castingCount
        ReceiverController.screens
        return ReceiverController.castReplacedBy(requestId)
    }

    width: 380
    implicitHeight: content.implicitHeight + 36
    height: implicitHeight
    radius: Theme.radius
    color: Theme.surfaceRaised
    border.color: Theme.border

    ColumnLayout {
        id: content

        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            Rectangle {
                implicitWidth: 44
                implicitHeight: 44
                radius: 22
                color: "#263ec6e0"

                Image {
                    anchors.centerIn: parent
                    source: Qt.resolvedUrl("icons/smartphone.svg")
                    sourceSize: Qt.size(22, 22)
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    text: root.screenNumber > 0
                          ? qsTr("<b>%1</b> wants to cast to screen %2").arg(root.deviceName).arg(root.screenNumber)
                          : qsTr("<b>%1</b> wants to cast").arg(root.deviceName)
                    textFormat: Text.StyledText
                    color: Theme.text
                    font.pixelSize: 16
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: root.deviceModel + "  ·  " + root.address
                    color: Theme.textMuted
                    font.pixelSize: 13
                    elide: Text.ElideRight
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: root.replaces.length > 0
            text: qsTr("Allowing ends the current cast from %1.").arg(root.replaces)
            color: Theme.warning
            font.pixelSize: 13
            wrapMode: Text.Wrap
        }

        CheckBox {
            id: alwaysAllow

            Layout.leftMargin: -6
            text: qsTr("Always allow this device")
            font.pixelSize: 14
            focusPolicy: Qt.TabFocus
            palette.base: Theme.bg
            palette.mid: Theme.textFaint
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Label {
                Layout.fillWidth: true
                text: qsTr("Expires in %1 s").arg(root.secondsLeft)
                color: Theme.textFaint
                font.pixelSize: 12
            }

            PillButton {
                text: qsTr("Decline")
                onClicked: ReceiverController.decline(root.requestId)
            }

            PillButton {
                kind: "primary"
                text: qsTr("Allow")
                onClicked: ReceiverController.accept(root.requestId, alwaysAllow.checked)
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: Theme.radius
        anchors.bottomMargin: 1
        height: 2
        radius: 1
        color: Theme.accent
        opacity: 0.7
        width: (parent.width - 2 * Theme.radius) * root.secondsLeft / ReceiverController.requestTimeoutSeconds

        Behavior on width {
            NumberAnimation { duration: 1000 }
        }
    }
}
