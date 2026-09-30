pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Sender

// First-launch introduction: what the phone app does, and the desktop app
// it pairs with. Also reachable again from About.
Item {
    id: root

    readonly property string repoUrl: "https://github.com/mjsolidarios/beamr"
    readonly property bool lastPage: pages.currentIndex === pages.count - 1

    signal finished()

    // Back steps through the pages; false on the first one.
    function goBack() {
        if (pages.currentIndex === 0)
            return false
        pages.decrementCurrentIndex()
        return true
    }

    readonly property var content: [
        {
            art: 0,
            title: qsTr("Your phone, on the big screen"),
            body: qsTr("Cast your Android screen to a computer on your Wi‑Fi, smooth at 60 frames a second. "
                       + "Show it on up to four computers at once.")
        },
        {
            art: 1,
            title: qsTr("Scan to connect"),
            body: qsTr("Open beamr on your computer and scan the code it shows. No accounts, no cables, "
                       + "and computers you've used stay one tap away.")
        },
        {
            art: 2,
            title: qsTr("Bring the sound along"),
            body: qsTr("Share what your apps play: videos, games, music. Calls and apps that block "
                       + "capture stay private.")
        },
        {
            art: 3,
            title: qsTr("Meet the desktop app"),
            body: qsTr("beamr for Linux and Windows shows up to four phones side by side. Pause, record, "
                       + "take screenshots, and pick whose sound plays.")
        }
    ]

    Rectangle {
        anchors.fill: parent
        color: Theme.bg
    }

    // Swallow taps so nothing underneath reacts.
    MouseArea {
        anchors.fill: parent
    }

    AbstractButton {
        id: skipButton

        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: Theme.gutter - 8
        implicitHeight: Theme.touchTarget
        leftPadding: 12
        rightPadding: 12
        opacity: root.lastPage ? 0 : 1
        enabled: !root.lastPage
        onClicked: root.finished()
        Accessible.name: qsTr("Skip introduction")

        Behavior on opacity { NumberAnimation { duration: 150 } }

        contentItem: Label {
            text: qsTr("Skip")
            color: Theme.textMuted
            font.pixelSize: 16
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: height / 2
            color: skipButton.down ? Theme.pressed : "transparent"
        }
    }

    SwipeView {
        id: pages

        anchors.top: skipButton.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: footer.top

        Repeater {
            model: root.content

            delegate: Item {
                id: page

                required property var modelData
                required property int index

                // Pinned at the same height on every page, so titles line up
                // as you swipe.
                ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: Theme.gutter + 8
                    anchors.rightMargin: Theme.gutter + 8
                    anchors.topMargin: Math.max(16, parent.height * 0.12)
                    spacing: 0

                    OnboardingArt {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: Math.min(280, page.width - 64)
                        Layout.preferredHeight: Layout.preferredWidth * 0.82
                        kind: page.modelData.art
                        running: pages.currentIndex === page.index && root.visible
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.topMargin: 40
                        horizontalAlignment: Text.AlignHCenter
                        text: page.modelData.title
                        color: Theme.text
                        font.pixelSize: 28
                        font.weight: Font.Bold
                        wrapMode: Text.Wrap
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        horizontalAlignment: Text.AlignHCenter
                        text: page.modelData.body
                        color: Theme.textMuted
                        font.pixelSize: 16
                        lineHeight: 1.2
                        wrapMode: Text.Wrap
                    }

                    // The one page with somewhere to go.
                    AbstractButton {
                        id: desktopLink

                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 16
                        visible: page.modelData.art === 3
                        implicitHeight: Theme.touchTarget
                        leftPadding: 14
                        rightPadding: 14
                        onClicked: Qt.openUrlExternally(root.repoUrl + "/releases")

                        contentItem: RowLayout {
                            spacing: 8

                            Label {
                                text: qsTr("Get the desktop app")
                                color: Theme.accent
                                font.pixelSize: 16
                                font.weight: Font.DemiBold
                            }

                            Icon {
                                glyph: "external-link"
                                size: 16
                                color: Theme.accent
                            }
                        }
                        background: Rectangle {
                            radius: height / 2
                            color: desktopLink.down ? Theme.pressed : "transparent"
                        }
                    }
                }
            }
        }
    }

    ColumnLayout {
        id: footer

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.gutter
        spacing: 24

        // Page dots; the current one stretches into a pill.
        Row {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8

            Repeater {
                model: pages.count

                delegate: Rectangle {
                    required property int index
                    readonly property bool current: index === pages.currentIndex

                    width: current ? 24 : 8
                    height: 8
                    radius: 4
                    color: current ? Theme.accent : Theme.border

                    Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                    Behavior on color { ColorAnimation { duration: 200 } }
                }
            }
        }

        PillButton {
            Layout.fillWidth: true
            kind: "primary"
            text: root.lastPage ? qsTr("Get started") : qsTr("Next")
            onClicked: root.lastPage ? root.finished() : pages.incrementCurrentIndex()
        }
    }
}
