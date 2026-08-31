import QtQuick
import QtQuick.Layouts
import ArchTitan.Media 1.0

/**
 * ExpandedView — Full media island with large art, metadata, progress,
 * waveform visualization, transport controls, and audio profile.
 */
Item {
    id: expandedRoot

    signal collapseRequested()

    // Click empty area to collapse
    MouseArea {
        anchors.fill: parent
        onClicked: expandedRoot.collapseRequested()
        cursorShape: Qt.PointingHandCursor
        z: -1
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        // ── TOP ROW: ART + METADATA ──────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            AlbumArt {
                Layout.preferredWidth: 82
                Layout.preferredHeight: 82
                Layout.alignment: Qt.AlignTop
                artSize: 82
                cornerRadius: 16
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 3

                // Source player badge
                Rectangle {
                    height: 16
                    width: badgeText.implicitWidth + 10
                    radius: 8
                    color: "#1889B4FA"
                    border.color: "#3089B4FA"
                    border.width: 1

                    Text {
                        id: badgeText
                        anchors.centerIn: parent
                        text: (Mpris && Mpris.playerName ? Mpris.playerName : "PLAYER").toUpperCase()
                        font.family: "JetBrainsMono Nerd Font"
                        font.pixelSize: 8
                        font.weight: Font.Bold
                        font.letterSpacing: 0.8
                        color: "#89B4FA"
                    }
                }

                // Title
                Text {
                    id: titleText
                    Layout.fillWidth: true
                    text: Mpris ? Mpris.title : "No Media"
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 14
                    font.weight: Font.Bold
                    color: "#CDD6F4"
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    opacity: titleFader.fadeValue

                    QtObject {
                        id: titleFader
                        property real fadeValue: 1.0
                    }

                    // Subtle crossfade on track change
                    Behavior on text {
                        SequentialAnimation {
                            NumberAnimation { target: titleFader; property: "fadeValue"; to: 0.3; duration: 80 }
                            PropertyAction {}
                            NumberAnimation { target: titleFader; property: "fadeValue"; to: 1.0; duration: 150 }
                        }
                    }
                }

                // Artist
                Text {
                    Layout.fillWidth: true
                    text: Mpris ? Mpris.artist : ""
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 11
                    color: "#A6ADC8"
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                // Album
                Text {
                    Layout.fillWidth: true
                    text: Mpris && Mpris.album ? Mpris.album : ""
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 10
                    color: "#585B70"
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    visible: text.length > 0
                }
            }
        }

        // ── TIME LABELS ──────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true

            Text {
                text: Mpris ? Mpris.positionText : "0:00"
                font.family: "JetBrainsMono Nerd Font"
                font.pixelSize: 10
                font.weight: Font.DemiBold
                color: "#89B4FA"
            }

            Item { Layout.fillWidth: true }

            Text {
                text: Mpris ? Mpris.lengthText : "0:00"
                font.family: "JetBrainsMono Nerd Font"
                font.pixelSize: 10
                color: "#585B70"
                visible: Mpris ? Mpris.length > 0 : false
            }
        }

        // ── PROGRESS VISUALIZATION (Novel Concept) ───────────────────────
        ProgressViz {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
        }

        // ── TRANSPORT CONTROLS ───────────────────────────────────────────
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 16

            // Shuffle
            TransportButton {
                size: 26
                iconText: "🔀"
                iconSize: 12
                isActive: Mpris ? Mpris.shuffle : false
                onActivated: if (Mpris) Mpris.toggleShuffle()
            }

            TransportButton {
                size: 30
                iconText: "⏮"
                iconSize: 12
                onActivated: if (Mpris) Mpris.previous()
            }

            TransportButton {
                size: 38
                iconText: (Mpris && Mpris.isPlaying) ? "❚❚" : "▶"
                iconSize: (Mpris && Mpris.isPlaying) ? 14 : 16
                isPrimary: true
                onActivated: if (Mpris) Mpris.playPause()
            }

            TransportButton {
                size: 30
                iconText: "⏭"
                iconSize: 12
                onActivated: if (Mpris) Mpris.next()
            }

            // Loop
            TransportButton {
                size: 26
                iconText: Mpris && Mpris.loopStatus === "Track" ? "🔂" : "🔁"
                iconSize: 12
                isActive: Mpris ? Mpris.loopStatus !== "None" : false
                onActivated: if (Mpris) Mpris.cycleLoop()
            }
        }

        // ── BOTTOM: AUDIO PROFILE ────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            visible: AudioProfile.hasProfile

            Rectangle {
                height: 16
                width: profileText.implicitWidth + 16
                radius: 8
                color: "#1074C7EC"
                border.color: "#2074C7EC"
                border.width: 1

                Text {
                    id: profileText
                    anchors.centerIn: parent
                    text: "🎵 " + AudioProfile.profileName
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 9
                    color: "#74C7EC"
                }
            }

            Item { Layout.fillWidth: true }
        }
    }
}
