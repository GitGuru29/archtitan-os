import QtQuick
import ArchTitan.Media 1.0

/**
 * ProgressViz — "Pulse River" progress visualization.
 *
 * Renders the track timeline as a luminous flowing constellation / river
 * of glowing nodes. Nodes behind the playhead glow sapphire with dynamic
 * ripple waves. Includes interactive drag/seek and hover preview tooltip.
 */
Item {
    id: vizRoot

    readonly property int dotCount: 44
    readonly property real progress: Mpris ? Math.max(0.0, Math.min(1.0, Mpris.progress)) : 0.0
    readonly property bool playing: Mpris ? Mpris.isPlaying : false

    // Animated phase for the wave ripples
    property real pulsePhase: 0.0

    NumberAnimation on pulsePhase {
        running: vizRoot.playing
        from: 0.0; to: Math.PI * 2
        duration: 2200
        loops: Animation.Infinite
    }

    // ── SEEK INTERACTION ─────────────────────────────────────────────────
    MouseArea {
        id: seekArea
        anchors.fill: parent
        anchors.topMargin: -6
        anchors.bottomMargin: -6
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

    // ── BACKGROUND TRACK LINE ────────────────────────────────────────────
    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.right: parent.right
        height: 2
        radius: 1
        color: "#18FFFFFF"
    }

    // ── PLAYED TRACK PROGRESS LINE ───────────────────────────────────────
    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        width: Math.max(0, vizRoot.width * vizRoot.progress)
        height: 2
        radius: 1
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#74C7EC" }
            GradientStop { position: 1.0; color: "#89B4FA" }
        }
    }

    // ── PULSE RIVER DOTS ─────────────────────────────────────────────────
    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 2
        anchors.rightMargin: 2
        spacing: Math.max(2, (vizRoot.width - 4 - (vizRoot.dotCount * 3.5)) / Math.max(1, vizRoot.dotCount - 1))

        Repeater {
            model: vizRoot.dotCount

            Rectangle {
                id: dot
                width: 3.5
                radius: 1.75
                anchors.verticalCenter: parent ? parent.verticalCenter : undefined

                readonly property real dotFrac: (index + 0.5) / vizRoot.dotCount
                readonly property bool isPlayed: dotFrac <= vizRoot.progress
                readonly property bool isNearHead: Math.abs(dotFrac - vizRoot.progress) < 0.08
                readonly property real distFromHead: Math.abs(dotFrac - vizRoot.progress)

                // Height wave
                height: {
                    if (isNearHead) return 14
                    if (!vizRoot.playing) return isPlayed ? 6 : 3

                    var wave = Math.sin(vizRoot.pulsePhase - index * 0.35) * 0.5 + 0.5
                    if (isPlayed) {
                        return 4 + wave * 7  // 4 - 11px
                    } else {
                        return 2 + wave * 2  // 2 - 4px
                    }
                }

                color: {
                    if (isNearHead) return "#FFFFFF"
                    if (isPlayed) {
                        return (dotFrac / Math.max(0.01, vizRoot.progress) < 0.5) ? "#74C7EC" : "#89B4FA"
                    }
                    return distFromHead < 0.15 ? "#585B70" : "#313244"
                }

                opacity: {
                    if (isNearHead) return 1.0
                    if (isPlayed) return 0.6 + (1.0 - distFromHead) * 0.4
                    return 0.35 + (vizRoot.playing ? Math.sin(vizRoot.pulsePhase + index * 0.3) * 0.15 : 0)
                }

                Behavior on height { NumberAnimation { duration: 160; easing.type: Easing.OutQuad } }
                Behavior on color { ColorAnimation { duration: 180 } }
            }
        }
    }

    // ── RADIANT PLAYHEAD BEACON ──────────────────────────────────────────
    Rectangle {
        id: playheadBeacon
        width: 10
        height: 10
        radius: 5
        color: "#FFFFFF"
        x: Math.max(0, Math.min(vizRoot.width - width, (vizRoot.progress * vizRoot.width) - (width / 2)))
        anchors.verticalCenter: parent.verticalCenter
        border.color: "#89B4FA"
        border.width: 2

        // Outer glow
        Rectangle {
            anchors.centerIn: parent
            width: parent.width + 8
            height: parent.height + 8
            radius: width / 2
            color: "transparent"
            border.color: "#6089B4FA"
            border.width: 1.5
            visible: vizRoot.playing

            SequentialAnimation on scale {
                running: vizRoot.playing
                loops: Animation.Infinite
                NumberAnimation { from: 1.0; to: 1.6; duration: 900; easing.type: Easing.InOutQuad }
                NumberAnimation { from: 1.6; to: 1.0; duration: 900; easing.type: Easing.InOutQuad }
            }
            SequentialAnimation on opacity {
                running: vizRoot.playing
                loops: Animation.Infinite
                NumberAnimation { from: 0.8; to: 0.1; duration: 900; easing.type: Easing.InOutQuad }
                NumberAnimation { from: 0.1; to: 0.8; duration: 900; easing.type: Easing.InOutQuad }
            }
        }
    }

    // ── SEEK PREVIEW LINE & HOVER GLOW ───────────────────────────────────
    Rectangle {
        width: 2
        height: parent.height + 4
        radius: 1
        color: "#B4BEFE"
        visible: seekArea.containsMouse
        x: Math.max(0, Math.min(vizRoot.width - width, seekArea.mouseX - 1))
        anchors.verticalCenter: parent.verticalCenter
        opacity: 0.7
    }
}
