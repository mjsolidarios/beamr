pragma Singleton
import QtQuick
import Beamr.Receiver

// Dark by default, with a light variant; follows the system unless Settings
// picks one. The video area stays black either way.
QtObject {
    readonly property bool dark: ReceiverController.colorScheme === Qt.Dark
                                 || (ReceiverController.colorScheme !== Qt.Light
                                     && Application.styleHints.colorScheme !== Qt.Light)

    readonly property color bg: dark ? "#0f1115" : "#f4f6f9"
    readonly property color surface: dark ? "#171a21" : "#ffffff"
    readonly property color surfaceRaised: dark ? "#1f232c" : "#ffffff"
    readonly property color border: dark ? "#2a2f3a" : "#dde2ea"
    readonly property color hover: dark ? "#1affffff" : "#0f000000"
    readonly property color pressed: dark ? "#2effffff" : "#1a000000"
    // Controls floating over the picture.
    readonly property color overlayBar: dark ? "#eb171a21" : "#f2ffffff"

    readonly property color text: dark ? "#e8eaf0" : "#131722"
    readonly property color textMuted: dark ? "#9aa3b2" : "#566071"
    readonly property color textFaint: dark ? "#5c6473" : "#8a93a3"

    // Deeper cyan on light backgrounds keeps text and borders legible.
    readonly property color accent: dark ? "#3ec6e0" : "#0b8ca6"
    readonly property color accentSoft: dark ? "#263ec6e0" : "#1a0b8ca6"
    readonly property color accentInk: dark ? "#06242b" : "#ffffff"
    readonly property color danger: dark ? "#ef5350" : "#d6342f"
    readonly property color record: dark ? "#ff4757" : "#e0283a"
    readonly property color warning: dark ? "#f5b942" : "#a86a12"
    readonly property color success: dark ? "#3ecf8e" : "#138a55"
    // The "Add screen" button: a quiet green, stronger on hover.
    readonly property color successSoft: dark ? "#143ecf8e" : "#12138a55"
    readonly property color successHover: dark ? "#243ecf8e" : "#20138a55"
    readonly property color successPressed: dark ? "#343ecf8e" : "#30138a55"
    readonly property color successBorder: dark ? "#663ecf8e" : "#66138a55"

    readonly property int radius: 12
    readonly property int radiusSmall: 8

    readonly property string monoFamily: Qt.platform.os === "windows" ? "Consolas" : "monospace"

    function formatDuration(totalSeconds) {
        const h = Math.floor(totalSeconds / 3600)
        const m = Math.floor(totalSeconds / 60) % 60
        const s = totalSeconds % 60
        const mm = (h > 0 && m < 10 ? "0" : "") + m
        const ss = (s < 10 ? "0" : "") + s
        return h > 0 ? h + ":" + mm + ":" + ss : mm + ":" + ss
    }
}
