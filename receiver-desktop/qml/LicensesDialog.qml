pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Receiver

// License texts stored in the app, so this opens with no network.
Dialog {
    id: root

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(720, parent ? parent.width - 48 : 720)
    height: Math.min(560, parent ? parent.height - 48 : 560)
    modal: true
    title: qsTr("Licenses")
    padding: 16
    topPadding: 8

    readonly property var licenses: ReceiverController.openSourceLicenses()

    header: Label {
        text: root.title
        color: Theme.text
        font.pixelSize: 20
        font.weight: Font.DemiBold
        leftPadding: 24
        topPadding: 22
        bottomPadding: 10
    }

    background: Rectangle {
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border
    }

    contentItem: RowLayout {
        spacing: 12

        ListView {
            id: licenseList

            Layout.preferredWidth: 230
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: root.licenses.length
            currentIndex: 0

            delegate: ItemDelegate {
                id: row

                required property int index

                width: ListView.view.width
                padding: 8
                Accessible.name: root.licenses[index].title
                onClicked: licenseList.currentIndex = index

                background: Rectangle {
                    radius: Theme.radiusSmall
                    color: row.index === licenseList.currentIndex ? Theme.accentSoft
                         : row.hovered ? Theme.hover : "transparent"
                }

                contentItem: Text {
                    text: root.licenses[index].title
                    color: Theme.text
                    font.pixelSize: 13
                    wrapMode: Text.Wrap
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Theme.radiusSmall
            color: Theme.bg
            border.color: Theme.border

            ScrollView {
                id: scroller

                anchors.fill: parent
                anchors.margins: 12
                clip: true

                TextArea {
                    readOnly: true
                    selectByMouse: true
                    persistentSelection: true
                    wrapMode: TextEdit.Wrap
                    text: {
                        if (licenseList.currentIndex < 0 || licenseList.currentIndex >= root.licenses.length)
                            return ""
                        const body = ReceiverController.openSourceLicense(root.licenses[licenseList.currentIndex].id)
                        return body.length > 0 ? body : qsTr("This license text isn't available.")
                    }
                    width: scroller.availableWidth
                    color: Theme.text
                    selectionColor: Theme.accent
                    selectedTextColor: Theme.accentInk
                    font.pixelSize: 13
                    font.family: {
                        if (licenseList.currentIndex < 0 || licenseList.currentIndex >= root.licenses.length)
                            return Qt.application.font.family
                        const id = root.licenses[licenseList.currentIndex].id
                        return id === "notices" ? Qt.application.font.family : Theme.monoFamily
                    }
                    leftPadding: 0
                    rightPadding: 0
                    topPadding: 0
                    bottomPadding: 0
                    background: null
                }
            }
        }
    }

    footer: DialogButtonBox {
        alignment: Qt.AlignRight
        padding: 16

        background: Item {}

        PillButton {
            kind: "primary"
            text: qsTr("Done")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }
}
