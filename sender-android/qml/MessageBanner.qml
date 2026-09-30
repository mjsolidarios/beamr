import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Sender

// Outcome of the last attempt: why it failed, or how it ended, with the one
// action that recovers from it when there is one.
Rectangle {
    id: root

    property string text
    property bool isError
    property string actionText

    signal dismissed()
    signal actionTriggered()

    implicitHeight: row.implicitHeight + 24
    radius: Theme.radius
    color: isError ? Theme.dangerSoft : Theme.surface
    border.color: isError ? Theme.dangerBorder : Theme.border

    RowLayout {
        id: row

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 14
        spacing: 12

        Icon {
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 12
            glyph: root.isError ? "alert" : "info"
            size: 20
            color: root.isError ? Theme.danger : Theme.textMuted
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: 12
            Layout.bottomMargin: root.actionText.length > 0 ? 4 : 12
            spacing: 0

            Label {
                Layout.fillWidth: true
                text: root.text
                color: Theme.text
                font.pixelSize: 14
                lineHeight: 1.15
                wrapMode: Text.Wrap
            }

            // Text button, aligned with the message it acts on.
            ToolButton {
                id: actionButton

                Layout.leftMargin: -10
                visible: root.actionText.length > 0
                implicitHeight: Theme.touchTarget
                leftPadding: 10
                rightPadding: 10
                text: root.actionText
                font.pixelSize: 14
                font.weight: Font.DemiBold
                onClicked: root.actionTriggered()

                contentItem: Text {
                    text: actionButton.text
                    font: actionButton.font
                    color: Theme.accent
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: height / 2
                    color: actionButton.down ? Theme.pressed : "transparent"
                }
            }
        }

        ToolButton {
            Layout.alignment: Qt.AlignTop
            implicitWidth: Theme.touchTarget
            implicitHeight: Theme.touchTarget
            onClicked: root.dismissed()
            Accessible.name: qsTr("Dismiss")

            contentItem: Icon {
                glyph: "x"
                size: 16
                color: Theme.textMuted
            }
            background: null
        }
    }

    Accessible.role: Accessible.AlertMessage
    Accessible.name: text
}
