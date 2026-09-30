import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Sender

// Whether casts include what apps play. The same setting on the cast card
// and in Settings.
Switch {
    id: soundSwitch

    checked: SenderController.shareAudio
    onToggled: {
        SenderController.shareAudio = checked
        // Toggling replaces the binding; restore it so the other copy's
        // changes show here too.
        checked = Qt.binding(() => SenderController.shareAudio)
    }
    // The indicator sits on the right; the row reserves its space.
    leftPadding: 6
    rightPadding: 6
    topPadding: 6
    bottomPadding: 6
    Accessible.name: qsTr("Cast sound")

    contentItem: RowLayout {
        spacing: 12

        Icon {
            glyph: soundSwitch.checked ? "volume-2" : "volume-x"
            size: 20
            color: soundSwitch.checked ? Theme.accent : Theme.textMuted
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 1

            Label {
                text: qsTr("Cast sound")
                color: Theme.text
                font.pixelSize: Theme.sp(15)
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("What apps play. Calls and apps that block it stay private.")
                color: Theme.textMuted
                font.pixelSize: Theme.sp(12)
                wrapMode: Text.Wrap
            }
        }

        Item {
            implicitWidth: soundSwitch.indicator.width
        }
    }

    indicator: Rectangle {
        x: soundSwitch.width - width - soundSwitch.rightPadding
        y: (soundSwitch.height - height) / 2
        implicitWidth: 48
        implicitHeight: 28
        radius: height / 2
        color: soundSwitch.checked ? Theme.accent : Theme.border

        Behavior on color { ColorAnimation { duration: 150 } }

        Rectangle {
            x: soundSwitch.checked ? parent.width - width - 3 : 3
            anchors.verticalCenter: parent.verticalCenter
            width: 22
            height: 22
            radius: 11
            color: soundSwitch.checked ? Theme.accentInk : Theme.surface

            Behavior on x { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
        }
    }
}
