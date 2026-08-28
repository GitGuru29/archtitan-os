import QtQuick
import ArchTitan.Media 1.0

/**
 * ProgressViz — "Pulse River" progress visualization.
 *
 * A novel progress concept: instead of a standard linear bar, the track
 * is rendered as a flowing river of luminous dots/particles.
 *
 * - Each dot represents a small slice of the track.
 * - Dots behind the playhead glow bright sapphire with a trailing pulse wave.
 * - Dots ahead of the playhead are dim embers.
 * - A radiant "playhead beacon" marks the current position.
 * - The dots subtly breathe/pulse when music is playing.
 * - When paused, the river freezes.
 * - Click anywhere to seek.
 *
 * The overall effect is a living, breathing representation of the song's
 * journey — like watching a constellation trace across the sky.
 */
Item {
    id: vizRoot

    readonly property int dotCount: 48
    readonly property real progress: Mpris ? Mpris.progress : 0.0
    readonly property bool playing: Mpris ? Mpris.isPlaying : false
    readonly property real dotSpacing: (width - 8) / dotCount

    // Animated phase for the pulse wave
    property real pulsePhase: 0.0

    NumberAnimation on pulsePhase {
        running: vizRoot.playing
        from: 0.0; to: Math.PI * 2
        duration: 2000
        loops: Animation.Infinite
    }

    // ── SEEK INTERACTION LAYER ───────────────────────────────────────────
    MouseArea {
        id: seekArea
        anchors.fill: parent
        anchors.topMargin: -4
        anchors.bottomMargin: -4
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor

        onClicked: (mouse) => {
            if (!Mpris) return
            var frac = Math.max(0.0, Math.min(1.0, mouse.x / vizRoot.width))
            Mpris.seek(frac)
        }

        onPositionChanged: (mouse) => {
            if (pressed && Mpris) {
                var frac = Math.max(0.0, Math.min(1.0, mouse.x / vizRoot.width))
                Mpris.seek(frac)
            }
        }
    }

    // ── DOT RIVER ────────────────────────────────────────────────────────
    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 4
        anchors.rightMargin: 4
        spacing: (vizRoot.width - 8 - dotCount * 4) / Math.max(1, dotCount - 1)

        Repeater {
            model: vizRoot.dotCount

            Rectangle {
                id: dot
                width: 4
                radius: 2
                anchors.verticalCenter: parent ? parent.verticalCenter : undefined

                readonly property real dotFrac: (index + 0.5) / vizRoot.dotCount
                readonly property bool isPlayed: dotFrac <= vizRoot.progress
                readonly property bool isPlayhead: Math.abs(dotFrac - vizRoot.progress) < (1.0 / vizRoot.dotCount)
                readonly property real distFromHead: Math.abs(dotFrac - vizRoot.progress)

                // ── HEIGHT: Dots near playhead are taller, creating a wave crest
                height: {
                    if (isPlayhead) return 16
                    if (!vizRoot.playing) return isPlayed ? 6 : 3

                    // Sine wave emanating from playhead
                    var wave = Math.sin(vizRoot.pulsePhase - index * 0.4) * 0.5 + 0.5
                    if (isPlayed) {
                        return 4 + wave * 8  // 4-12px, flowing
                    } else {
                        return 2 + wave * 2  // 2-4px, subtle shimmer
                    }
                }

                // ── COLOR: Played = bright sapphire gradient, Unplayed = dim ember
                color: {
                    if (isPlayhead) return "#FFFFFF"  // Beacon white
                    if (isPlayed) {
                        // Gradient from sky blue (start) to sapphire (near playhead)
                        var playedFrac = dotFrac / Math.max(0.01, vizRoot.progress)
                        if (playedFrac < 0.5) return "#74C7EC"  // Catppuccin Sky
                        return "#89B4FA"  // Catppuccin Sapphire
                    }
                    // Unplayed: dark ember
                    return distFromHead < 0.1 ? "#45475A" : "#313244"
                }

                // ── OPACITY: Pulse wave near playhead
                opacity: {
                    if (isPlayhead) return 1.0
                    if (isPlayed) {
                        // Trail glow: brighter closer to playhead
                        return 0.5 + (1.0 - distFromHead) * 0.5
                    }
                    // Unplayed: faint
                    return 0.3 + (vizRoot.playing ? Math.sin(vizRoot.pulsePhase + index * 0.3) * 0.15 : 0)
                }

                // ── HOVER: Highlight dots on hover for seek preview
                Rectangle {
                    anchors.fill: parent
                    radius: parent.radius
                    color: "#89B4FA"
                    opacity: seekArea.containsMouse &&
                             Math.abs(dotFrac - (seekArea.mouseX / vizRoot.width)) < (1.5 / vizRoot.dotCount)
                             ? 0.6 : 0.0

                    Behavior on opacity { NumberAnimation { duration: 80 } }
                }

                Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.OutQuad } }
                Behavior on color { ColorAnimation { duration: 200 } }
                Behavior on opacity { NumberAnimation { duration: 150 } }
            }
        }
    }

    // ── PLAYHEAD GLOW ────────────────────────────────────────────────────
    Rectangle {
        id: playheadGlow
        width: 8
        height: 8
        radius: 4
        color: "#89B4FA"
        opacity: vizRoot.playing ? 0.4 : 0.0
        x: Math.max(4, Math.min(vizRoot.width - 12, vizRoot.progress * vizRoot.width - 4))
        anchors.verticalCenter: parent.verticalCenter

        // Pulsing glow
        SequentialAnimation on opacity {
            running: vizRoot.playing
            loops: Animation.Infinite
            NumberAnimation { from: 0.2; to: 0.6; duration: 800; easing.type: Easing.InOutQuad }
            NumberAnimation { from: 0.6; to: 0.2; duration: 800; easing.type: Easing.InOutQuad }
        }

        SequentialAnimation on scale {
            running: vizRoot.playing
            loops: Animation.Infinite
            NumberAnimation { from: 1.0; to: 2.5; duration: 800; easing.type: Easing.InOutQuad }
            NumberAnimation { from: 2.5; to: 1.0; duration: 800; easing.type: Easing.InOutQuad }
        }
    }

    // ── SEEK PREVIEW LINE ────────────────────────────────────────────────
    Rectangle {
        width: 1
        height: parent.height
        color: "#4089B4FA"
        visible: seekArea.containsMouse
        x: Math.max(0, Math.min(vizRoot.width - 1, seekArea.mouseX))
    }
}
