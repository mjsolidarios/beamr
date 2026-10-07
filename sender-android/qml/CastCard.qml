import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Sender

// The one screen capture: ready to start, waiting on the consent dialog, or
// live to some number of receivers.
Rectangle {
    id: root

    readonly property int castState: SenderController.castState
    // changingShare is the gap between stopping this capture and the new
    // consent dialog. Treat it as Starting so the card doesn't flash Live.
    readonly property bool starting: castState === SenderController.Starting || SenderController.changingShare
    readonly property bool live: castState === SenderController.On && !SenderController.changingShare
    readonly property int receivers: live ? SenderController.streamingCount : SenderController.approvedCount

    implicitHeight: content.implicitHeight + 40
    radius: Theme.radius
    color: live ? Theme.successSoft : Theme.surface
    border.color: live ? Theme.success : Theme.border

    Behavior on color { ColorAnimation { duration: 200 } }

    ColumnLayout {
        id: content

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 20
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            Rectangle {
                implicitWidth: 44
                implicitHeight: 44
                radius: 22
                color: root.live ? Theme.success : Theme.accentSoft

                Icon {
                    anchors.centerIn: parent
                    glyph: "cast"
                    size: 22
                    color: root.live ? Theme.bg : Theme.accent
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                RowLayout {
                    spacing: 6

                    // Live light, like a recording dot.
                    Rectangle {
                        visible: root.live
                        implicitWidth: 8
                        implicitHeight: 8
                        radius: 4
                        color: Theme.success

                        SequentialAnimation on opacity {
                            running: root.live && root.visible
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.3; duration: 800 }
                            NumberAnimation { to: 1; duration: 800 }
                        }
                    }

                    Label {
                        text: root.live ? qsTr("Live")
                            : root.starting ? qsTr("Starting")
                            : qsTr("Ready")
                        color: root.live ? Theme.success : Theme.accent
                        font.pixelSize: Theme.sp(12)
                        font.weight: Font.DemiBold
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: 1.2
                    }
                }

                Label {
                    Layout.fillWidth: true
                    text: root.live && root.receivers === 0 && SenderController.reconnectingCount > 0
                          ? qsTr("Reconnecting…")
                        : root.live ? (root.receivers === 1 ? qsTr("Casting to 1 receiver")
                                                            : qsTr("Casting to %1 receivers").arg(root.receivers))
                        : root.starting ? qsTr("Allow screen sharing")
                        : root.receivers === 1 ? qsTr("Cast to 1 receiver")
                        : qsTr("Cast to %1 receivers").arg(root.receivers)
                    color: Theme.text
                    font.pixelSize: Theme.sp(18)
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 14
            text: root.live && SenderController.shareTarget === "app"
                  ? qsTr("That app stays on the computer while it's open. Stop here or from the notification.")
                  : root.live
                    ? qsTr("Switch to any app. Stop here or from the notification.")
                    : root.starting
                      ? qsTr("In the dialog that opens, share your entire screen or just one app, then allow it.")
                      : qsTr("Your screen shows on every receiver below that allowed this phone.")
            color: Theme.textMuted
            font.pixelSize: Theme.sp(14)
            lineHeight: 1.15
            wrapMode: Text.Wrap
        }

        // What this cast is actually sharing, beside a small frame of it.
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 14
            visible: root.live && SenderController.castSize.width > 0
            spacing: 12

            Rectangle {
                readonly property real aspect: SenderController.castSize.height > 0
                                                ? SenderController.castSize.width / SenderController.castSize.height
                                                : 1
                readonly property int frameWidth: Math.round(Math.min(200, (aspect < 1 ? 120 : 72) * aspect))
                readonly property int frameHeight: Math.max(1, Math.round(frameWidth / aspect))

                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: frameWidth
                Layout.preferredHeight: frameHeight
                implicitWidth: frameWidth
                implicitHeight: frameHeight
                radius: Theme.radiusSmall
                color: Theme.bg
                border.color: Theme.border
                clip: true

                Image {
                    anchors.fill: parent
                    visible: SenderController.previewRevision > 0
                    cache: false
                    mipmap: true
                    asynchronous: false
                    fillMode: Image.PreserveAspectFit
                    source: visible ? "image://beamrpreview/" + SenderController.previewRevision : ""
                }

                Icon {
                    anchors.centerIn: parent
                    visible: SenderController.previewRevision === 0
                    glyph: SenderController.shareTarget === "app" ? "smartphone"
                         : SenderController.shareTarget === "screen" ? "monitor" : "cast"
                    size: 22
                    color: Theme.textFaint
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    Layout.fillWidth: true
                    visible: text.length > 0
                    text: SenderController.shareTarget === "app" ? qsTr("One app")
                        : SenderController.shareTarget === "screen" ? qsTr("Entire screen") : ""
                    color: Theme.text
                    font.pixelSize: Theme.sp(14)
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    text: {
                        const size = qsTr("%1 × %2 · %3 fps").arg(SenderController.castSize.width)
                                                              .arg(SenderController.castSize.height)
                                                              .arg(SenderController.frameRate)
                        return SenderController.audioState === "on" ? qsTr("%1 · with sound").arg(size) : size
                    }
                    color: Theme.textFaint
                    font.pixelSize: Theme.sp(13)
                    font.features: { "tnum": 1 }
                    wrapMode: Text.Wrap
                }
            }
        }

        // Why there's no sound, when it was asked for.
        Label {
            Layout.fillWidth: true
            Layout.topMargin: 6
            visible: root.live && (SenderController.audioState === "denied"
                                   || SenderController.audioState === "unavailable")
            text: SenderController.audioState === "denied"
                  ? qsTr("No sound yet. Allow audio for beamr in Android settings. It is only used for what apps play.")
                  : qsTr("This phone can't cast sound, so only the picture is shared.")
            color: Theme.textMuted
            font.pixelSize: Theme.sp(13)
            wrapMode: Text.Wrap
        }

        PillButton {
            Layout.fillWidth: true
            Layout.topMargin: 12
            visible: root.live && SenderController.audioState === "denied"
            kind: "secondary"
            text: qsTr("Open settings")
            onClicked: SenderController.openAppSettings()
        }

        // The stream is skipping frames. Data saver is the one step down.
        Label {
            Layout.fillWidth: true
            Layout.topMargin: 14
            visible: root.live && SenderController.networkStruggling
            text: SenderController.castQuality === SenderController.DataSaver
                  ? qsTr("Wi-Fi is dropping frames, so the picture may stutter.")
                  : qsTr("Wi-Fi is dropping frames.")
            color: Theme.textMuted
            font.pixelSize: Theme.sp(13)
            wrapMode: Text.Wrap
        }

        PillButton {
            Layout.fillWidth: true
            Layout.topMargin: 12
            visible: root.live && SenderController.networkStruggling
                     && SenderController.castQuality !== SenderController.DataSaver
            kind: "secondary"
            text: qsTr("Use Data saver")
            onClicked: SenderController.useDataSaver()
        }

        // Sound is chosen before casting; changing it on a live cast waits for Apply.
        SoundSwitch {
            Layout.fillWidth: true
            Layout.topMargin: 12
            Layout.leftMargin: -6
            Layout.rightMargin: -6
            visible: !root.live && !root.starting
        }

        Label {
            Layout.fillWidth: true
            Layout.topMargin: 14
            visible: SenderController.castSettingsPending
            text: qsTr("Picture or sound settings changed.")
            color: Theme.textMuted
            font.pixelSize: Theme.sp(13)
            wrapMode: Text.Wrap
        }

        PillButton {
            Layout.fillWidth: true
            Layout.topMargin: 12
            visible: SenderController.castSettingsPending
            kind: "secondary"
            text: qsTr("Apply to this cast")
            onClicked: SenderController.applyCastSettings()
        }

        PillButton {
            Layout.fillWidth: true
            Layout.topMargin: 16
            visible: !root.starting
            kind: root.live ? "danger" : "primary"
            text: root.live ? qsTr("Stop casting") : qsTr("Start casting")
            onClicked: root.live ? SenderController.stopCasting() : SenderController.startCasting()
        }

        PillButton {
            Layout.fillWidth: true
            Layout.topMargin: 10
            visible: root.live
            kind: "secondary"
            text: qsTr("Change what's shared")
            onClicked: SenderController.changeShareTarget()
        }
    }
}
