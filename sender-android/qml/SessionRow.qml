import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Sender

// One connected receiver and where it's at.
ItemDelegate {
    id: root

    required property int index
    required property string sessionId
    required property string name
    required property string address
    required property int sessionState

    readonly property bool pending: sessionState === SenderController.Connecting
                                    || sessionState === SenderController.AwaitingApproval
                                    || sessionState === SenderController.Reconnecting
    readonly property bool streaming: sessionState === SenderController.Streaming
    readonly property color tone: streaming ? Theme.success : pending ? Theme.accent : Theme.textMuted

    leftPadding: 16
    rightPadding: 4
    topPadding: 12
    bottomPadding: 12
    // Rows aren't tappable; only their remove button is.
    hoverEnabled: false
    down: false

    background: Rectangle {
        color: "transparent"

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
            color: root.streaming ? Theme.successSoft : Theme.accentSoft

            Icon {
                anchors.centerIn: parent
                glyph: "monitor"
                size: 18
                color: root.streaming ? Theme.success : Theme.accent
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: root.name
                color: Theme.text
                font.pixelSize: Theme.sp(16)
                elide: Text.ElideRight
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Rectangle {
                    Layout.alignment: root.sessionState === SenderController.AwaitingApproval ? Qt.AlignTop
                                                                                              : Qt.AlignVCenter
                    Layout.topMargin: root.sessionState === SenderController.AwaitingApproval ? 5 : 0
                    implicitWidth: 6
                    implicitHeight: 6
                    radius: 3
                    color: root.tone

                    // Breathes while we wait on someone else.
                    SequentialAnimation on opacity {
                        running: root.pending && root.visible
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.25; duration: 600 }
                        NumberAnimation { to: 1; duration: 600 }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: {
                        switch (root.sessionState) {
                        case SenderController.Connecting: return qsTr("Connecting…")
                        case SenderController.AwaitingApproval:
                            return qsTr("Tap Allow in the beamr window. If it's closed, tap the beamr icon on the computer's panel.")
                        case SenderController.Reconnecting: return qsTr("Reconnecting…")
                        // The address tells apart computers with the same name.
                        case SenderController.Streaming: return qsTr("Live · %1").arg(root.address)
                        default: return qsTr("Connected · %1").arg(root.address)
                        }
                    }
                    color: root.pending || root.streaming ? root.tone : Theme.textMuted
                    font.pixelSize: Theme.sp(13)
                    wrapMode: root.sessionState === SenderController.AwaitingApproval ? Text.Wrap : Text.NoWrap
                    maximumLineCount: root.sessionState === SenderController.AwaitingApproval ? 4 : 1
                    elide: root.sessionState === SenderController.AwaitingApproval ? Text.ElideNone : Text.ElideRight
                }
            }
        }

        ToolButton {
            implicitWidth: Theme.touchTarget
            implicitHeight: Theme.touchTarget
            onClicked: SenderController.removeReceiver(root.sessionId)
            Accessible.name: root.pending ? qsTr("Cancel connecting to %1").arg(root.name)
                                          : qsTr("Disconnect from %1").arg(root.name)

            contentItem: Icon {
                glyph: "x"
                size: 18
                color: Theme.textMuted
            }
            background: Rectangle {
                radius: width / 2
                color: parent.down ? Theme.pressed : "transparent"
            }
        }
    }
}
