import QtQuick
import QtQuick.Layouts
import ArchTitan.Media 1.0

/**
 * CompactView — Small pill showing art, title·artist, mini controls, time.
 * Click anywhere (except controls) to expand.
 */
Item {
    id: compactRoot

    signal expandRequested()

    // Tap to expand
    MouseArea {
        anchors.fill: parent
        onClicked: compactRoot.expandRequested()
        cursorShape: Qt.PointingHandCursor
    }

    RowLayout {
        anchors.fill: parent
        spacing: 10

        // ── ALBUM ART (compact) ──────────────────────────────────────────
        AlbumArt {
            Layout.preferredWidth: 42
            Layout.preferredHeight: 42
            Layout.alignment: Qt.AlignVCenter
            artSize: 42
            cornerRadius: 10
        }

        // ── TITLE · ARTIST ───────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 1

            Text {
                Layout.fillWidth: true
                text: Mpris ? Mpris.title : "No Media"
                font.family: "JetBrainsMono Nerd Font"
                font.pixelSize: 12
                font.weight: Font.DemiBold
                color: "#CDD6F4"
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            Text {
                Layout.fillWidth: true
                text: Mpris ? Mpris.artist : ""
                font.family: "JetBrainsMono Nerd Font"
                font.pixelSize: 10
                color: "#A6ADC8"
                elide: Text.ElideRight
                maximumLineCount: 1
                visible: text.length > 0
            }
        }

        // ── MINI WAVEFORM (playing indicator) ────────────────────────────
        Row {
            spacing: 2
            Layout.alignment: Qt.AlignVCenter
            visible: Mpris && Mpris.isPlaying

            Repeater {
                model: 3
                Rectangle {
                    width: 2.5
                    radius: 1.25
                    color: "#89B4FA"
                    height: 8
                    anchors.verticalCenter: parent ? parent.verticalCenter : undefined

                    SequentialAnimation on height {
                        running: Mpris && Mpris.isPlaying
                        loops: Animation.Infinite
                        NumberAnimation {
                            from: 3 + index * 2
                            to: 11 - index
                            duration: 300 + index * 80
                            easing.type: Easing.InOutSine
                        }
                        NumberAnimation {
                            from: 11 - index
                            to: 3 + index * 2
                            duration: 350 + index * 60
                            easing.type: Easing.InOutSine
                        }
                    }
                }
            }
        }

        // ── MINI TRANSPORT ───────────────────────────────────────────────
        Row {
            spacing: 4
            Layout.alignment: Qt.AlignVCenter

            TransportButton {
                size: 24
                iconText: "⏮"
                iconSize: 9
                onActivated: if (Mpris) Mpris.previous()
            }

            TransportButton {
                size: 28
                iconText: (Mpris && Mpris.isPlaying) ? "❚❚" : "▶"
                iconSize: (Mpris && Mpris.isPlaying) ? 10 : 11
                isPrimary: true
                onActivated: if (Mpris) Mpris.playPause()
            }

            TransportButton {
                size: 24
                iconText: "⏭"
                iconSize: 9
                onActivated: if (Mpris) Mpris.next()
            }
        }

        // ── ELAPSED TIME ─────────────────────────────────────────────────
        Text {
            text: Mpris ? Mpris.positionText : "0:00"
            font.family: "JetBrainsMono Nerd Font"
            font.pixelSize: 9
            color: "#585B70"
            Layout.alignment: Qt.AlignVCenter
            visible: Mpris ? Mpris.length > 0 : false
        }
    }
}
