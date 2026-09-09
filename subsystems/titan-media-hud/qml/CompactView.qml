import QtQuick
import QtQuick.Layouts
import ArchTitan.Media 1.0

/**
 * CompactView — Sleek, modern Dynamic Island pill view.
 * Shows album art, track title, artist, live equalizer bars,
 * mini transport controls, and an integrated bottom progress strip.
 * Click anywhere to expand into full control mode.
 */
Item {
    id: compactRoot

    signal expandRequested()

    // ── CLICK BACKGROUND TO EXPAND ───────────────────────────────────────
    MouseArea {
        anchors.fill: parent
        onClicked: compactRoot.expandRequested()
        cursorShape: Qt.PointingHandCursor
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 10
        anchors.topMargin: 4
        anchors.bottomMargin: 6
        spacing: 10

        // ── ALBUM ART ────────────────────────────────────────────────────
        AlbumArt {
            Layout.preferredWidth: 38
            Layout.preferredHeight: 38
            Layout.alignment: Qt.AlignVCenter
            artSize: 38
            cornerRadius: 9
            showGlow: true
        }

        // ── TITLE · ARTIST ───────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            Text {
                Layout.fillWidth: true
                text: Mpris && Mpris.title.length > 0 ? Mpris.title : "No Media Playing"
                font.family: "JetBrainsMono Nerd Font"
                font.pixelSize: 12
                font.weight: Font.DemiBold
                color: "#CDD6F4"
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                // Player icon badge
                Rectangle {
                    height: 14
                    width: playerTag.implicitWidth + 8
                    radius: 7
                    color: "#1889B4FA"
                    border.color: "#3089B4FA"
                    border.width: 1
                    visible: Mpris && Mpris.playerName.length > 0

                    Text {
                        id: playerTag
                        anchors.centerIn: parent
                        text: Mpris ? Mpris.playerName.toUpperCase() : ""
                        font.family: "JetBrainsMono Nerd Font"
                        font.pixelSize: 8
                        font.weight: Font.Bold
                        color: "#89B4FA"
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: Mpris && Mpris.artist.length > 0 ? Mpris.artist : "ArchTitan Audio"
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 10
                    color: "#A6ADC8"
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
            }
        }

        // ── LIVE FREQUENCY EQUALIZER BARS ────────────────────────────────
        Row {
            spacing: 2.5
            Layout.alignment: Qt.AlignVCenter
            visible: Mpris && Mpris.isPlaying

            Repeater {
                model: 4
                Rectangle {
                    width: 2.5
                    radius: 1.25
                    color: (index % 2 === 0) ? "#89B4FA" : "#74C7EC"
                    height: 8
                    anchors.verticalCenter: parent ? parent.verticalCenter : undefined

                    SequentialAnimation on height {
                        running: Mpris && Mpris.isPlaying
                        loops: Animation.Infinite
                        NumberAnimation {
                            from: 3 + (index * 2) % 6
                            to: 12 - (index * 2) % 5
                            duration: 260 + index * 60
                            easing.type: Easing.InOutQuad
                        }
                        NumberAnimation {
                            from: 12 - (index * 2) % 5
                            to: 3 + (index * 2) % 6
                            duration: 280 + index * 50
                            easing.type: Easing.InOutQuad
                        }
                    }
                }
            }
        }

        // ── MINI TRANSPORT CONTROLS ──────────────────────────────────────
        Row {
            spacing: 5
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

        // ── EXPAND CHEVRON ───────────────────────────────────────────────
        Text {
            text: "▾"
            font.pixelSize: 12
            color: "#585B70"
            Layout.alignment: Qt.AlignVCenter
            opacity: 0.7
        }
    }

    // ── BOTTOM INTEGRATED PROGRESS STRIP ─────────────────────────────────
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        height: 2
        radius: 1
        color: "#15FFFFFF"

        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: Math.max(0, parent.width * (Mpris ? Mpris.progress : 0.0))
            radius: 1
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#74C7EC" }
                GradientStop { position: 1.0; color: "#89B4FA" }
            }
        }
    }
}
