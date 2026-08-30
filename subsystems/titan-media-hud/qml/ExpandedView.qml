import QtQuick
import QtQuick.Layouts
import ArchTitan.Media 1.0

/**
 * ExpandedView — Full Dynamic Island card with rich album artwork,
 * comprehensive metadata, interactive Pulse River visualizer/seeker,
 * full transport deck, audio profile tags, and collapse action.
 */
Item {
    id: expandedRoot

    signal collapseRequested()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 10

        // ── TOP HEADER ROW: ARTWORK + TRACK METADATA + COLLAPSE BTN ─────
        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            // Large Album Art
            AlbumArt {
                Layout.preferredWidth: 68
                Layout.preferredHeight: 68
                Layout.alignment: Qt.AlignTop
                artSize: 68
                cornerRadius: 14
                showGlow: true
            }

            // Track Details
            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 3

                // Player Badge + Close Button Row
                RowLayout {
                    Layout.fillWidth: true

                    Rectangle {
                        height: 16
                        width: badgeText.implicitWidth + 12
                        radius: 8
                        color: "#1889B4FA"
                        border.color: "#3089B4FA"
                        border.width: 1

                        Text {
                            id: badgeText
                            anchors.centerIn: parent
                            text: (Mpris && Mpris.playerName.length > 0 ? Mpris.playerName : "MEDIA").toUpperCase()
                            font.family: "JetBrainsMono Nerd Font"
                            font.pixelSize: 8
                            font.weight: Font.Bold
                            font.letterSpacing: 0.8
                            color: "#89B4FA"
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Collapse button
                    Rectangle {
                        width: 22
                        height: 22
                        radius: 11
                        color: closeArea.containsMouse ? "#25FFFFFF" : "#10FFFFFF"

                        Text {
                            anchors.centerIn: parent
                            text: "✕"
                            font.pixelSize: 10
                            color: closeArea.containsMouse ? "#FFFFFF" : "#A6ADC8"
                        }

                        MouseArea {
                            id: closeArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: expandedRoot.collapseRequested()
                        }
                    }
                }

                // Track Title
                Text {
                    Layout.fillWidth: true
                    text: Mpris && Mpris.title.length > 0 ? Mpris.title : "No Media Playing"
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 14
                    font.weight: Font.Bold
                    color: "#FFFFFF"
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                // Artist
                Text {
                    Layout.fillWidth: true
                    text: Mpris && Mpris.artist.length > 0 ? Mpris.artist : "ArchTitan Audio"
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 11
                    color: "#BAC2DE"
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                // Album (if available)
                Text {
                    Layout.fillWidth: true
                    text: Mpris && Mpris.album.length > 0 ? Mpris.album : ""
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 10
                    color: "#6C7086"
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    visible: text.length > 0
                }
            }
        }

        // ── PROGRESS VISUALIZATION (Pulse River) ──────────────────────────
        ProgressViz {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
        }

        // ── TIMESTAMPS ROW ───────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: -4

            Text {
                text: Mpris ? Mpris.positionText : "0:00"
                font.family: "JetBrainsMono Nerd Font"
                font.pixelSize: 9.5
                font.weight: Font.DemiBold
                color: "#89B4FA"
            }

            Item { Layout.fillWidth: true }

            Text {
                text: Mpris ? Mpris.lengthText : "0:00"
                font.family: "JetBrainsMono Nerd Font"
                font.pixelSize: 9.5
                color: "#6C7086"
                visible: Mpris ? Mpris.length > 0 : false
            }
        }

        // ── FULL TRANSPORT CONTROLS ROW ──────────────────────────────────
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 18

            // Shuffle
            TransportButton {
                size: 28
                iconText: "🔀"
                iconSize: 11
                isActive: Mpris ? Mpris.shuffle : false
                onActivated: if (Mpris) Mpris.toggleShuffle()
            }

            // Previous
            TransportButton {
                size: 32
                iconText: "⏮"
                iconSize: 12
                onActivated: if (Mpris) Mpris.previous()
            }

            // Play / Pause (Large Primary Button)
            TransportButton {
                size: 42
                iconText: (Mpris && Mpris.isPlaying) ? "❚❚" : "▶"
                iconSize: (Mpris && Mpris.isPlaying) ? 14 : 16
                isPrimary: true
                onActivated: if (Mpris) Mpris.playPause()
            }

            // Next
            TransportButton {
                size: 32
                iconText: "⏭"
                iconSize: 12
                onActivated: if (Mpris) Mpris.next()
            }

            // Repeat / Loop
            TransportButton {
                size: 28
                iconText: Mpris && Mpris.loopStatus === "Track" ? "🔂" : "🔁"
                iconSize: 11
                isActive: Mpris ? Mpris.loopStatus !== "None" : false
                onActivated: if (Mpris) Mpris.cycleLoop()
            }
        }

        // ── FOOTER: AUDIO PROFILE BADGE ──────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            visible: AudioProfile.hasProfile

            Rectangle {
                height: 16
                width: profileText.implicitWidth + 14
                radius: 8
                color: "#1274C7EC"
                border.color: "#2574C7EC"
                border.width: 1

                Text {
                    id: profileText
                    anchors.centerIn: parent
                    text: "🎵 " + AudioProfile.profileName
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 8.5
                    color: "#74C7EC"
                }
            }

            Item { Layout.fillWidth: true }
        }
    }
}
