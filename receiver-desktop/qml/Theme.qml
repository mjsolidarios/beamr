pragma Singleton
import QtQuick

QtObject {
    readonly property color bg: "#0f1115"
    readonly property color surface: "#171a21"
    readonly property color surfaceRaised: "#1f232c"
    readonly property color border: "#2a2f3a"
    readonly property color hover: "#1affffff"
    readonly property color pressed: "#2effffff"

    readonly property color text: "#e8eaf0"
    readonly property color textMuted: "#9aa3b2"
    readonly property color textFaint: "#5c6473"

    readonly property color accent: "#3ec6e0"
    readonly property color accentInk: "#06242b"
    readonly property color danger: "#ef5350"
    readonly property color record: "#ff4757"
    readonly property color warning: "#f5b942"
    readonly property color success: "#3ecf8e"

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
