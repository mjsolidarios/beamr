pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Receiver

// A free screen among several: its own QR code, so a phone that scans it
// lands exactly here.
Rectangle {
    id: root

    required property CastScreen screen
    property bool removable

    signal removeRequested()

    radius: Theme.radius
    color: Theme.surface
    border.color: Theme.border

    IconButton {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        visible: root.removable
        iconName: "x"
        tint: Theme.textMuted
        tip: qsTr("Remove screen %1").arg(root.screen.number)
        onClicked: root.removeRequested()
    }

    ColumnLayout {
        id: column

        // Shrinks the code before anything else when the tile is small.
        readonly property real codeSize: Math.max(120, Math.min(220, root.height - 170, root.width - 64))

        anchors.centerIn: parent
        width: Math.min(root.width - 48, 320)
        spacing: 0

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Screen %1").arg(root.screen.number)
            color: Theme.success
            font.pixelSize: 12
            font.weight: Font.DemiBold
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 1.4
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 4
            text: qsTr("Waiting for a phone")
            color: Theme.text
            font.pixelSize: 20
            font.weight: Font.Light
        }

        // Dark on white whatever the theme: that's what scanners read best.
        // The tile's margin is the code's quiet zone.
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 16
            visible: root.screen.connectLink.length > 0
            implicitWidth: column.codeSize
            implicitHeight: column.codeSize
            radius: Theme.radiusSmall
            color: "white"

            QrCode {
                anchors.fill: parent
                anchors.margins: Math.round(parent.width * 0.07)
                text: root.screen.connectLink
                color: "black"
            }

            Accessible.role: Accessible.Graphic
            Accessible.name: qsTr("QR code to cast to screen %1").arg(root.screen.number)
        }

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 14
            horizontalAlignment: Text.AlignHCenter
            text: root.screen.connectLink.length > 0
                  ? qsTr("In beamr on your phone, tap Scan QR code.")
                  : qsTr("No network connection. Connect this computer to the same Wi-Fi as your phone.")
            color: root.screen.connectLink.length > 0 ? Theme.textMuted : Theme.warning
            font.pixelSize: 14
            wrapMode: Text.Wrap
        }
    }
}
