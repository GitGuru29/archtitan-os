import QtQuick
import ArchTitan.Media 1.0

/**
 * AlbumArt — Rounded album artwork with crossfade on track change,
 * breathing glow when playing, and fallback gradient.
 */
Item {
    id: artRoot

    property int artSize: 54
    property int cornerRadius: 14

    width: artSize
    height: artSize

    // ── FALLBACK (gradient + note icon) ──────────────────────────────────
    Rectangle {
        id: fallback
        anchors.fill: parent
        radius: artRoot.cornerRadius
        visible: !currentArt.visible || currentArt.status !== Image.Ready

        gradient: Gradient {
            GradientStop { position: 0.0; color: "#2A2A36" }
            GradientStop { position: 0.6; color: "#1E1E2E" }
            GradientStop { position: 1.0; color: "#14141A" }
        }

        Text {
            anchors.centerIn: parent
            text: "♫"
            font.pixelSize: Math.round(artRoot.artSize * 0.38)
            color: "#585B70"
        }
    }

    // ── OUTGOING ART (for crossfade) ─────────────────────────────────────
    Image {
        id: outgoingArt
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        mipmap: true
        visible: opacity > 0.01
        opacity: 0.0
        smooth: true

        // Clip to rounded rect
        layer.enabled: true
        layer.effect: Item {
            Rectangle {
                anchors.fill: parent
                radius: artRoot.cornerRadius
            }
        }
    }

    // ── CURRENT ART ──────────────────────────────────────────────────────
    Image {
        id: currentArt
        anchors.fill: parent
        source: Mpris ? Mpris.artUrl : ""
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        mipmap: true
        visible: status === Image.Ready
        smooth: true

        // Clip to rounded rect
        layer.enabled: true
        layer.effect: Item {
            Rectangle {
                anchors.fill: parent
                radius: artRoot.cornerRadius
            }
        }

        // Crossfade on source change
        onSourceChanged: {
            if (outgoingArt.source.toString().length > 0) {
                outgoingArt.source = outgoingArt.source // keep old
                outgoingArt.opacity = 1.0
                crossfadeOut.start()
            }
        }

        NumberAnimation {
            id: crossfadeOut
            target: outgoingArt
            property: "opacity"
            from: 1.0; to: 0.0
            duration: 300
            easing.type: Easing.OutQuad
        }
    }

    // ── BORDER ───────────────────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        radius: artRoot.cornerRadius
        color: "transparent"
        border.color: (Mpris && Mpris.isPlaying) ? "#3089B4FA" : "#18FFFFFF"
        border.width: 1
    }

    // ── BREATHING GLOW (playing) ─────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        radius: artRoot.cornerRadius
        color: "transparent"
        border.color: "#6089B4FA"
        border.width: 1.5
        visible: Mpris && Mpris.isPlaying
        opacity: 0.4

        SequentialAnimation on opacity {
            running: Mpris && Mpris.isPlaying
            loops: Animation.Infinite
            NumberAnimation { from: 0.15; to: 0.6; duration: 1400; easing.type: Easing.InOutQuad }
            NumberAnimation { from: 0.6; to: 0.15; duration: 1400; easing.type: Easing.InOutQuad }
        }
    }
}
