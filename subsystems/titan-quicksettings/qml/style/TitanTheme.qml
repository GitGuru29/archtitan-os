pragma Singleton
import QtQuick

QtObject {
    // ── Colors: Catppuccin Mocha & Obsidian Base ────────────────────────────────
    readonly property color bgObsidian: "#F011141D"       // 94% opacity deep glass
    readonly property color bgCard: "#141722"           // Sub-card background
    readonly property color bgCardHover: "#1E2230"      // Sub-card hover state
    readonly property color bgPillInactive: "#1F2330"   // Inactive pill tile
    readonly property color bgButtonDark: "#1A1D27"     // Action button background
    readonly property color bgButtonHover: "#2A3042"    // Action button hover

    // ── Accents ────────────────────────────────────────────────────────────────
    readonly property color accentCyan: "#38BDF8"       // Electric cyan active highlight
    readonly property color accentCyanDark: "#0A1322"   // Dark text on active cyan background
    readonly property color accentRed: "#F87171"        // Danger / power red
    readonly property color accentRedBg: "#3A2024"      // Power button hover bg

    // ── Text & Borders ─────────────────────────────────────────────────────────
    readonly property color textPrimary: "#FFFFFF"
    readonly property color textSecondary: "#CBD5E1"
    readonly property color textMuted: "#94A3B8"
    readonly property color textSubtle: "#64748B"

    readonly property color borderCyanGlow: "#2E38BDF8" // Subtle cyan border
    readonly property color borderGlassLight: "#18FFFFFF"
    readonly property color borderGlassHighlight: "#25FFFFFF"

    // ── Typography & Metrics ───────────────────────────────────────────────────
    readonly property string fontFamily: "Outfit, Inter, sans-serif"
    readonly property int radiusWindow: 22
    readonly property int radiusCard: 18
    readonly property int radiusPill: 23
    readonly property int radiusButton: 16
}
