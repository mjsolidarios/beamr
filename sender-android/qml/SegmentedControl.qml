pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Sender

// One choice of a few, as a pill of segments: [{value, label, glyph?}].
Rectangle {
    id: root

    property var options: []
    property var value

    signal activated(var value)

    implicitHeight: 52
    radius: height / 2
    color: Theme.bg
    border.color: Theme.border

    RowLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        Repeater {
            model: root.options

            delegate: AbstractButton {
                id: segment

                required property var modelData
                readonly property bool selected: root.value === modelData.value

                Layout.fillWidth: true
                Layout.fillHeight: true
                checkable: true
                checked: selected
                onClicked: root.activated(modelData.value)
                Accessible.name: modelData.label
                Accessible.role: Accessible.RadioButton

                background: Rectangle {
                    radius: height / 2
                    color: segment.selected ? Theme.accentSoft
                         : segment.down ? Theme.pressed : "transparent"
                    border.width: segment.selected ? 1 : 0
                    border.color: Theme.accent

                    Behavior on color { ColorAnimation { duration: 150 } }
                }

                contentItem: Item {
                    Row {
                        anchors.centerIn: parent
                        spacing: 8

                        Icon {
                            anchors.verticalCenter: parent.verticalCenter
                            visible: (segment.modelData.glyph ?? "").length > 0
                            glyph: segment.modelData.glyph ?? ""
                            size: 18
                            color: segment.selected ? Theme.accent : Theme.textMuted
                        }

                        Label {
                            anchors.verticalCenter: parent.verticalCenter
                            text: segment.modelData.label
                            color: segment.selected ? Theme.accent : Theme.text
                            font.pixelSize: 15
                            font.weight: segment.selected ? Font.DemiBold : Font.Normal
                        }
                    }
                }
            }
        }
    }
}
