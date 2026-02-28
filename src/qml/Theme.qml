pragma Singleton
import QtQuick

QtObject {
    // === Colors (UI-01) ===
    readonly property color primaryBg: "#0a1628"
    readonly property color secondaryBg: "#162844"
    readonly property color accent: "#2e5c8a"
    readonly property color accentLight: "#4a8cc7"
    readonly property color textPrimary: "#e8edf3"
    readonly property color textSecondary: "#8899aa"
    readonly property color textDimmed: "#556677"
    readonly property color glassBg: Qt.rgba(0.086, 0.157, 0.267, 0.75)
    readonly property color glassBorder: Qt.rgba(0.18, 0.36, 0.54, 0.3)
    readonly property color successColor: "#2ecc71"
    readonly property color errorColor: "#e74c3c"
    readonly property color warningColor: "#f39c12"

    // === Typography (UI-02) ===
    readonly property int fontSizeSmall: 12
    readonly property int fontSizeBody: 16
    readonly property int fontSizeMedium: 20
    readonly property int fontSizeLarge: 28
    readonly property int fontSizeXLarge: 36
    readonly property int fontSizeHuge: 48

    // === Spacing (UI-02) ===
    readonly property int spacingSmall: 8
    readonly property int spacingMedium: 16
    readonly property int spacingLarge: 24
    readonly property int spacingXLarge: 32

    // === Touch Targets (UI-02) ===
    readonly property int touchTargetSmall: 44
    readonly property int touchTargetLarge: 64
    readonly property int touchTargetXLarge: 80

    // === Animation Durations (UI-02) ===
    readonly property int animFast: 150
    readonly property int animMedium: 300
    readonly property int animSlow: 500

    // === Layout ===
    readonly property int leftPanelWidth: 280
    readonly property int statusBarHeight: 56
    readonly property int radiusSmall: 8
    readonly property int radiusMedium: 16
    readonly property int radiusLarge: 24

    // === Dynamic Accent (set from album art) ===
    property color dynamicAccent: accent
}
