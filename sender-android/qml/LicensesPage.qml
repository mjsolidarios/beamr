pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Sender

// License texts stored in the app, so this page works with no network.
Item {
    id: root

    signal backRequested()

    property string licenseId: ""
    property string licenseTitle: ""
    readonly property var licenses: SenderController.openSourceLicenses()

    RowLayout {
        id: header

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: Theme.gutter - 12
        anchors.rightMargin: Theme.gutter
        height: 56
        spacing: 4

        ToolButton {
            id: backButton

            implicitWidth: Theme.touchTarget
            implicitHeight: Theme.touchTarget
            Accessible.name: qsTr("Back")
            onClicked: {
                if (root.licenseId.length > 0) {
                    root.licenseId = ""
                    root.licenseTitle = ""
                } else {
                    root.backRequested()
                }
            }

            contentItem: Icon {
                glyph: "arrow-left"
                size: 22
                color: Theme.text
            }
            background: Rectangle {
                radius: width / 2
                color: backButton.down ? Theme.pressed : "transparent"
            }
        }

        Label {
            Layout.fillWidth: true
            text: root.licenseId.length === 0 ? qsTr("Licenses") : root.licenseTitle
            color: Theme.text
            font.pixelSize: Theme.sp(20)
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }
    }

    Flickable {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: root.licenseId.length === 0
        contentWidth: width
        contentHeight: column.implicitHeight + 2 * Theme.gutter
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        ColumnLayout {
            id: column

            x: Theme.gutter
            y: Theme.gutter / 2
            width: parent.width - 2 * Theme.gutter
            spacing: 0

            Label {
                Layout.fillWidth: true
                Layout.bottomMargin: 16
                text: qsTr("Stored in the app, so they open without a network connection.")
                color: Theme.textMuted
                font.pixelSize: Theme.sp(14)
                lineHeight: 1.15
                wrapMode: Text.Wrap
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: licenseColumn.implicitHeight
                radius: Theme.radius
                color: Theme.surface
                border.color: Theme.border
                clip: true

                ColumnLayout {
                    id: licenseColumn

                    width: parent.width
                    spacing: 0

                    Repeater {
                        model: root.licenses.length

                        delegate: ItemDelegate {
                            id: row

                            required property int index

                            readonly property string title: root.licenses[index].title

                            Layout.fillWidth: true
                            leftPadding: 16
                            rightPadding: 16
                            topPadding: 14
                            bottomPadding: 14
                            Accessible.name: title
                            onClicked: {
                                root.licenseId = root.licenses[index].id
                                root.licenseTitle = title
                            }

                            background: Rectangle {
                                color: row.down ? Theme.pressed : "transparent"

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.leftMargin: 16
                                    height: 1
                                    visible: row.index > 0
                                    color: Theme.border
                                }
                            }

                            contentItem: RowLayout {
                                spacing: 14

                                Label {
                                    Layout.fillWidth: true
                                    text: row.title
                                    color: Theme.text
                                    font.pixelSize: Theme.sp(16)
                                    wrapMode: Text.Wrap
                                }

                                Icon {
                                    glyph: "chevron-right"
                                    size: 18
                                    color: Theme.textFaint
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    ScrollView {
        id: scroller

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.gutter
        visible: root.licenseId.length > 0
        clip: true

        TextArea {
            readOnly: true
            selectByMouse: true
            persistentSelection: true
            wrapMode: TextEdit.Wrap
            text: {
                const body = SenderController.openSourceLicense(root.licenseId)
                return body.length > 0 ? body : qsTr("This license text isn't available.")
            }
            width: scroller.availableWidth
            color: Theme.text
            selectionColor: Theme.accent
            selectedTextColor: Theme.accentInk
            font.pixelSize: Theme.sp(root.licenseId === "notices" ? 14 : 12)
            font.family: root.licenseId === "notices" ? Qt.application.font.family : Theme.monoFamily
            leftPadding: 0
            rightPadding: 0
            topPadding: 0
            background: null
        }
    }
}
