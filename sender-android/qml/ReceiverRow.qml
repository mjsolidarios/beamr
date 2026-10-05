import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Sender

// A computer the phone can connect to with one tap: nearby or recent.
ItemDelegate {
    id: root

    required property int index
    property string name
    property string address
    // Under the name; the address when empty and different from the name.
    property string detail
    // Recent computers can be removed. Nearby ones come and go on their own.
    property bool forgettable: false

    signal forgetRequested()

    Layout.fillWidth: true
    leftPadding: 16
    rightPadding: 12
    topPadding: 14
    bottomPadding: 14
    Accessible.name: qsTr("Connect to %1").arg(name)

    background: Rectangle {
        color: root.down ? Theme.pressed : "transparent"

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 66
            height: 1
            visible: root.index > 0
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
                text: root.name
                color: Theme.text
                font.pixelSize: Theme.sp(16)
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: root.detail.length > 0 ? root.detail : root.name !== root.address ? root.address : ""
                color: Theme.textMuted
                font.pixelSize: Theme.sp(13)
                font.family: root.detail.length > 0 ? Qt.application.font.family : Theme.monoFamily
                elide: Text.ElideRight
            }
        }

        // Recent rows: a real control, so forgetting isn't a hidden gesture.
        // It takes the click so the row doesn't connect as well.
        ToolButton {
            id: forgetButton

            visible: root.forgettable
            text: qsTr("Forget")
            font.pixelSize: Theme.sp(14)
            font.weight: Font.DemiBold
            padding: 0
            leftPadding: 10
            rightPadding: 10
            implicitHeight: Theme.touchTarget
            Accessible.name: qsTr("Forget %1").arg(root.name)
            onClicked: root.forgetRequested()

            contentItem: Text {
                text: forgetButton.text
                font: forgetButton.font
                color: Theme.danger
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                radius: height / 2
                color: forgetButton.down ? Theme.pressed : "transparent"
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
            font.pixelSize: Theme.sp(14)
            font.weight: Font.DemiBold

            background: Rectangle {
                radius: height / 2
                color: Theme.accentSoft
            }
        }
    }
}
