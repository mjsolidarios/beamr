import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Receiver

// What the receiver is doing across its screens.
Rectangle {
    id: root

    readonly property int castingCount: ReceiverController.castingCount
    readonly property bool casting: castingCount > 0
    // The one screen in use, when there's exactly one.
    readonly property CastScreen soleCast: {
        if (castingCount !== 1)
            return null
        return ReceiverController.screens.find(s => s.casting) ?? null
    }
    readonly property color dotColor: !casting ? Theme.success
                                    : soleCast && soleCast.paused ? Theme.warning
                                    : Theme.accent

    implicitWidth: row.implicitWidth + 24
    implicitHeight: 30
    radius: height / 2
    color: Theme.surface
    border.color: Theme.border

    RowLayout {
        id: row

        anchors.centerIn: parent
        spacing: 8

        Rectangle {
            implicitWidth: 8
            implicitHeight: 8
            radius: 4
            color: root.dotColor

            SequentialAnimation on opacity {
                running: !root.casting
                loops: Animation.Infinite
                alwaysRunToEnd: true
                NumberAnimation { to: 0.35; duration: 900; easing.type: Easing.InOutSine }
                NumberAnimation { to: 1; duration: 900; easing.type: Easing.InOutSine }
            }
        }

        Label {
            text: !root.casting ? qsTr("Ready")
                : !root.soleCast ? qsTr("Casting from %1 phones").arg(root.castingCount)
                : root.soleCast.paused ? qsTr("Paused · %1").arg(root.soleCast.deviceName)
                : qsTr("Casting from %1").arg(root.soleCast.deviceName)
            color: Theme.text
            font.pixelSize: 13
        }
    }
}
