pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Beamr.Sender

// What beamr is, where its source lives, the desktop app to pair it with,
// and the licenses it's built on.
Item {
    id: root

    required property string appVersion

    signal backRequested()
    signal introductionRequested()
    signal licensesRequested()

    readonly property string repoUrl: "https://github.com/mjsolidarios/beamr"

    // A tappable row: icon, title, subtitle, and a trailing hint.
    component LinkRow: ItemDelegate {
        id: row

        property string glyph
        property string title
        property string subtitle
        property bool external: true
        property bool first

        Layout.fillWidth: true
        leftPadding: 16
        rightPadding: 16
        topPadding: 14
        bottomPadding: 14
        Accessible.name: title

        background: Rectangle {
            color: row.down ? Theme.pressed : "transparent"

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 66
                height: 1
                visible: !row.first
                color: Theme.border
            }
        }

        contentItem: RowLayout {
            spacing: 14

            Rectangle {
                implicitWidth: 36
                implicitHeight: 36
                radius: 18
                color: Theme.accentSoft

                Icon {
                    anchors.centerIn: parent
                    glyph: row.glyph
                    size: 18
                    color: Theme.accent
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 1

                Label {
                    Layout.fillWidth: true
                    text: row.title
                    color: Theme.text
                    font.pixelSize: Theme.sp(16)
                    elide: Text.ElideRight
                }

                Label {
                    Layout.fillWidth: true
                    visible: row.subtitle.length > 0
                    text: row.subtitle
                    color: Theme.textMuted
                    font.pixelSize: Theme.sp(13)
                    elide: Text.ElideRight
                }
            }

            Icon {
                glyph: row.external ? "external-link" : "chevron-right"
                size: 18
                color: Theme.textFaint
            }
        }
    }

    component Card: Rectangle {
        default property alias rows: cardColumn.data

        Layout.fillWidth: true
        implicitHeight: cardColumn.implicitHeight
        radius: Theme.radius
        color: Theme.surface
        border.color: Theme.border
        clip: true

        ColumnLayout {
            id: cardColumn

            width: parent.width
            spacing: 0
        }
    }

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
            onClicked: root.backRequested()
            Accessible.name: qsTr("Back")

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
            text: qsTr("About")
            color: Theme.text
            font.pixelSize: Theme.sp(20)
            font.weight: Font.DemiBold
        }
    }

    Flickable {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
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

            // The icon keeps its own dark tile in both themes.
            Image {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 8
                Layout.preferredWidth: 96
                Layout.preferredHeight: 96
                // Pre-rounded, like a launcher icon.
                source: Qt.resolvedUrl("images/beamr-icon.png")
                sourceSize: Qt.size(192, 192)
                smooth: true
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 16
                text: "beamr"
                color: Theme.text
                font.pixelSize: Theme.sp(28)
                font.weight: Font.Bold
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 2
                text: qsTr("Version %1").arg(root.appVersion)
                color: Theme.textMuted
                font.pixelSize: Theme.sp(14)
            }

            Label {
                Layout.fillWidth: true
                Layout.topMargin: 12
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Cast your phone's screen and sound to your computers over Wi-Fi.")
                color: Theme.text
                font.pixelSize: Theme.sp(15)
                lineHeight: 1.15
                wrapMode: Text.Wrap
            }

            Card {
                Layout.topMargin: 28

                LinkRow {
                    first: true
                    glyph: "code"
                    title: qsTr("Source code on GitHub")
                    subtitle: "github.com/mjsolidarios/beamr"
                    onClicked: Qt.openUrlExternally(root.repoUrl)
                }

                LinkRow {
                    glyph: "monitor"
                    title: qsTr("Get the desktop app")
                    subtitle: qsTr("For Windows, macOS, and Linux")
                    onClicked: Qt.openUrlExternally(root.repoUrl + "/releases")
                }

                LinkRow {
                    glyph: "bug"
                    title: qsTr("Report a problem")
                    subtitle: qsTr("Opens GitHub issues")
                    onClicked: Qt.openUrlExternally(root.repoUrl + "/issues")
                }

                LinkRow {
                    glyph: "info"
                    title: qsTr("Privacy policy")
                    subtitle: qsTr("What the apps keep, and what they send")
                    onClicked: Qt.openUrlExternally("https://mjsolidarios.github.io/beamr/privacy.html")
                }

                LinkRow {
                    glyph: "book-open"
                    title: qsTr("Show the introduction")
                    external: false
                    onClicked: root.introductionRequested()
                }
            }

            SectionLabel {
                Layout.topMargin: 28
                text: qsTr("Open source")
            }

            Label {
                Layout.fillWidth: true
                Layout.topMargin: 8
                text: qsTr("beamr is free and open source under the MIT license. It's built with Qt %1, "
                           + "used under the GNU LGPL v3, and developed in Qt Creator.").arg(SenderController.qtVersion)
                color: Theme.textMuted
                font.pixelSize: Theme.sp(14)
                lineHeight: 1.15
                wrapMode: Text.Wrap
            }

            Card {
                Layout.topMargin: 14

                LinkRow {
                    first: true
                    glyph: "scale"
                    title: qsTr("Licenses")
                    subtitle: qsTr("beamr, Qt and the other open-source parts")
                    external: false
                    onClicked: root.licensesRequested()
                }
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 24
                text: qsTr("© 2026 Mark Joseph Solidarios")
                color: Theme.textFaint
                font.pixelSize: Theme.sp(12)
            }
        }
    }
}
