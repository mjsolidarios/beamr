pragma Singleton
import QtQuick

// The receiver's palette, plus a light variant; follows the system setting
// unless Settings picks one (SenderController.colorScheme).
//
// Shape rule: actions are pills, cards and banners use `radius`, inputs use
// `radiusSmall`. Cyan is the one accent; green only ever means "live".
QtObject {
    readonly property bool dark: Application.styleHints.colorScheme !== Qt.Light

    readonly property color bg: dark ? "#0f1115" : "#f4f6f9"
    readonly property color surface: dark ? "#171a21" : "#ffffff"
    readonly property color surfaceRaised: dark ? "#1f232c" : "#ffffff"
    readonly property color border: dark ? "#2a2f3a" : "#dde2ea"
    readonly property color pressed: dark ? "#2effffff" : "#14000000"

    readonly property color text: dark ? "#e8eaf0" : "#131722"
    readonly property color textMuted: dark ? "#9aa3b2" : "#566071"
    readonly property color textFaint: dark ? "#5c6473" : "#8a93a3"

    // Deeper cyan on light backgrounds keeps text and borders legible.
    readonly property color accent: dark ? "#3ec6e0" : "#0b8ca6"
    readonly property color accentSoft: dark ? "#263ec6e0" : "#1a0b8ca6"
    readonly property color accentInk: dark ? "#06242b" : "#ffffff"
    readonly property color danger: dark ? "#ef5350" : "#d6342f"
    readonly property color dangerSoft: dark ? "#26ef5350" : "#14d6342f"
    readonly property color dangerBorder: dark ? "#66ef5350" : "#59d6342f"
    readonly property color success: dark ? "#3ecf8e" : "#138a55"
    readonly property color successSoft: dark ? "#263ecf8e" : "#17138a55"

    readonly property int radius: 16
    readonly property int radiusSmall: 12
    // Material's minimum comfortable touch target.
    readonly property int touchTarget: 48
    readonly property int gutter: 20

    readonly property string monoFamily: "monospace"

    // Text sizes in pixels at normal size, scaled by Android's font size
    // setting. Use for every font.pixelSize.
    function sp(px) {
        return Math.round(px * SenderController.fontScale)
    }
}
