import QtQuick
import ArchTitan.Media 1.0

/**
 * AlbumArt — Rounded album artwork with smooth crossfade,
 * breathing ambient glow when playing, and sleek fallback illustration.
 */
Item {
    id: artRoot

    property int artSize: 44
    property int cornerRadius: 10
    property bool showGlow: true

    width: artSize
    height: artSize

    // ── AMBIENT GLOW RING (behind art when playing) ──────────────────────
    Rectangle {
        id: ambientGlow
        anchors.centerIn: parent
        width: artRoot.artSize + 6
        height: artRoot.artSize + 6
        radius: artRoot.cornerRadius + 3
        color: "transparent"
        border.color: "#6089B4FA"
        border.width: 1.5
        visible: artRoot.showGlow && (Mpris && Mpris.isPlaying)
        opacity: 0.0

        SequentialAnimation on opacity {
            running: artRoot.showGlow && (Mpris && Mpris.isPlaying)
            loops: Animation.Infinite
            NumberAnimation { from: 0.2; to: 0.7; duration: 1200; easing.type: Easing.InOutQuad }
            NumberAnimation { from: 0.7; to: 0.2; duration: 1200; easing.type: Easing.InOutQuad }
        }
    }

    // ── CLIPPED ART CONTAINER ────────────────────────────────────────────
    Rectangle {
        id: artContainer
        anchors.fill: parent
        radius: artRoot.cornerRadius
        clip: true
        color: "#181825"
        border.color: (Mpris && Mpris.isPlaying) ? "#3589B4FA" : "#20FFFFFF"
        border.width: 1

        // ── FALLBACK (sleek dark gradient + music icon) ──────────────────
        Rectangle {
            id: fallback
            anchors.fill: parent
            radius: artRoot.cornerRadius
            visible: !currentArt.visible || currentArt.status !== Image.Ready

            gradient: Gradient {
                GradientStop { position: 0.0; color: "#2E303E" }
                GradientStop { position: 0.5; color: "#1E1E2E" }
                GradientStop { position: 1.0; color: "#11111B" }
            }

            Text {
                anchors.centerIn: parent
                text: "♫"
                font.pixelSize: Math.max(12, Math.round(artRoot.artSize * 0.36))
                color: (Mpris && Mpris.isPlaying) ? "#89B4FA" : "#585B70"
                opacity: 0.85

                // Subtle pulse when playing without art
                SequentialAnimation on scale {
                    running: fallback.visible && (Mpris && Mpris.isPlaying)
                    loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 1.15; duration: 800; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 1.15; to: 1.0; duration: 800; easing.type: Easing.InOutQuad }
                }
            }
        }

        // ── OUTGOING ART (for crossfade) ─────────────────────────────────
        Image {
            id: outgoingArt
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            mipmap: true
            visible: opacity > 0.01
            opacity: 0.0
            smooth: true
        }

        // ── CURRENT ART ──────────────────────────────────────────────────
        Image {
            id: currentArt
            anchors.fill: parent
            source: Mpris ? Mpris.artUrl : ""
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            mipmap: true
            visible: status === Image.Ready
            smooth: true

            onSourceChanged: {
                if (outgoingArt.source.toString().length > 0) {
                    outgoingArt.source = outgoingArt.source
                    outgoingArt.opacity = 1.0
                    crossfadeOut.start()
                }
            }

            NumberAnimation {
                id: crossfadeOut
                target: outgoingArt
                property: "opacity"
                from: 1.0; to: 0.0
                duration: 250
                easing.type: Easing.OutQuad
            }
        }

        // ── INNER TOP SPECULAR HIGHLIGHT ─────────────────────────────────
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: "#25FFFFFF"
        }
    }
}
