import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Sender

ApplicationWindow {
    id: window

    required property string appVersion

    // Only used when the sender is built on desktop; Android ignores it.
    width: 400
    height: 800
    visible: true
    title: "beamr"
    color: Theme.bg

    palette.window: Theme.bg
    palette.base: Theme.surface
    palette.text: Theme.text
    palette.windowText: Theme.text
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.accentInk

    // Shows the introduction again (from About) after it was finished once.
    property bool replayIntroduction: false
    readonly property bool introductionShown: !SenderController.onboardingDone || replayIntroduction

    function finishIntroduction() {
        SenderController.onboardingDone = true
        replayIntroduction = false
    }

    // Back steps through the introduction and pages first. Past that,
    // quitting would drop every receiver and stop the cast; while connected,
    // Android's back gesture only sends the app to the background.
    onClosing: close => {
        if (introductionShown) {
            if (onboarding.item && onboarding.item.goBack())
                close.accepted = false
            return
        }
        if (pages.depth > 1) {
            close.accepted = false
            pages.pop()
            return
        }
        if (SenderController.sessions.count > 0) {
            close.accepted = false
            SenderController.moveToBackground()
        }
    }

    // The window draws edge to edge; keep content clear of the status and
    // navigation bars.
    Item {
        id: frame

        anchors.fill: parent

        Item {
            anchors.fill: parent
            anchors.topMargin: frame.SafeArea.margins.top
            anchors.bottomMargin: frame.SafeArea.margins.bottom
            anchors.leftMargin: frame.SafeArea.margins.left
            anchors.rightMargin: frame.SafeArea.margins.right

            StackView {
                id: pages

                anchors.fill: parent
                initialItem: homeScreen
            }

            // Full screen above everything until finished; fades out.
            Loader {
                id: onboarding

                anchors.fill: parent
                active: window.introductionShown || opacity > 0
                opacity: window.introductionShown ? 1 : 0
                z: 10

                Behavior on opacity { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }

                sourceComponent: OnboardingView {
                    onFinished: window.finishIntroduction()
                }
            }
        }
    }

    Component {
        id: homeScreen

        Item {
            RowLayout {
                id: topBar

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: Theme.gutter
                anchors.rightMargin: Theme.gutter
                height: 56

                Label {
                    text: "beamr"
                    color: Theme.text
                    font.pixelSize: Theme.sp(22)
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.3
                }

                Item { Layout.fillWidth: true }

                ToolButton {
                    id: settingsButton

                    Layout.rightMargin: -12
                    implicitWidth: Theme.touchTarget
                    implicitHeight: Theme.touchTarget
                    onClicked: settingsSheet.open()
                    Accessible.name: qsTr("Settings")

                    contentItem: Icon {
                        glyph: "settings"
                        size: 22
                        color: Theme.textMuted
                    }
                    background: Rectangle {
                        radius: width / 2
                        color: settingsButton.down ? Theme.pressed : "transparent"
                    }
                }
            }

            HomePage {
                anchors.top: topBar.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
            }
        }
    }

    Component {
        id: aboutScreen

        AboutPage {
            appVersion: window.appVersion
            onBackRequested: pages.pop()
            onIntroductionRequested: window.replayIntroduction = true
        }
    }

    SettingsSheet {
        id: settingsSheet

        appVersion: window.appVersion
        onRenameRequested: renameDialog.open()
        onAboutRequested: pages.push(aboutScreen)
    }

    Dialog {
        id: renameDialog

        anchors.centerIn: parent
        width: Math.min(parent.width - 2 * Theme.gutter, 400)
        modal: true
        padding: 24
        topPadding: 24
        onAboutToShow: nameField.text = SenderController.deviceName
        onOpened: nameField.forceActiveFocus()
        onAccepted: SenderController.deviceName = nameField.text

        Overlay.modal: Rectangle { color: Theme.dark ? "#99000000" : "#59000000" }

        background: Rectangle {
            radius: Theme.radius
            color: Theme.surfaceRaised
            border.color: Theme.border
        }

        contentItem: ColumnLayout {
            spacing: 0

            Label {
                text: qsTr("Phone name")
                color: Theme.text
                font.pixelSize: Theme.sp(20)
            }

            Label {
                Layout.fillWidth: true
                Layout.topMargin: 6
                text: qsTr("The computer shows this name when you ask to cast.")
                color: Theme.textMuted
                font.pixelSize: Theme.sp(14)
                wrapMode: Text.Wrap
            }

            TextField {
                id: nameField

                Layout.fillWidth: true
                Layout.topMargin: 18
                color: Theme.text
                font.pixelSize: Theme.sp(17)
                leftPadding: 14
                maximumLength: 64
                placeholderText: SenderController.deviceModel
                placeholderTextColor: Theme.textFaint
                onAccepted: renameDialog.accept()

                background: Rectangle {
                    implicitHeight: 52
                    radius: Theme.radiusSmall
                    color: Theme.surface
                    border.width: nameField.activeFocus ? 2 : 1
                    border.color: nameField.activeFocus ? Theme.accent : Theme.border
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 24
                spacing: 12

                PillButton {
                    Layout.fillWidth: true
                    text: qsTr("Cancel")
                    onClicked: renameDialog.reject()
                }

                PillButton {
                    Layout.fillWidth: true
                    kind: "primary"
                    text: qsTr("Save")
                    onClicked: renameDialog.accept()
                }
            }
        }
    }
}
