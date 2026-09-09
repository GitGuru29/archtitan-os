import QtQuick
import QtQuick.Layouts
import ArchTitan.Media 1.0
import ".."

/**
 * MediaPage — Primary HUD Carousel Page
 * Displays live MPRIS album art, track title, artist, album, live timestamp 1:27 / 3:22,
 * seekable progress bar, transport controls, and 4-bar dynamic audio visualizer.
 */
Item {
    id: mediaPageRoot
    anchors.fill: parent

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.topMargin: 6
        anchors.bottomMargin: 6
        spacing: 12

        // ── ALBUM ART ────────────────────────────────────────────────────
        AlbumArt {
            Layout.preferredWidth: 46
            Layout.preferredHeight: 46
            Layout.alignment: Qt.AlignVCenter
            artSize: 46
            cornerRadius: 10
            showGlow: true
        }

        // ── TRACK DETAILS & SEEKABLE PROGRESS BAR ────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            // Track Header: Tag + Title
            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Rectangle {
                    height: 14
                    width: playerTag.implicitWidth + 8
                    radius: 7
                    color: "#1889B4FA"
                    border.color: "#3089B4FA"
                    border.width: 1

                    Text {
                        id: playerTag
                        anchors.centerIn: parent
                        text: (Mpris && Mpris.playerName.length > 0 ? Mpris.playerName : "PLAYING").toUpperCase()
                        font.family: "JetBrainsMono Nerd Font"
                        font.pixelSize: 8
                        font.weight: Font.Bold
                        color: "#89B4FA"
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: Mpris && Mpris.title.length > 0 ? Mpris.title : "No Media Playing"
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 13
                    font.weight: Font.Bold
                    color: "#CDD6F4"
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
            }

            // Artist & Album
            Text {
                Layout.fillWidth: true
                text: (Mpris && Mpris.artist.length > 0 ? Mpris.artist : "ArchTitan Audio") +
                      (Mpris && Mpris.album.length > 0 ? " — " + Mpris.album : "")
                font.family: "JetBrainsMono Nerd Font"
                font.pixelSize: 11
                color: "#A6ADC8"
                elide: Text.ElideRight
                maximumLineCount: 1
            }

            // Timeline & Progress Bar
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                // Seekable Progress Line
                Rectangle {
                    Layout.fillWidth: true
                    height: 3
                    radius: 1.5
                    color: "#20FFFFFF"

                    Rectangle {
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        width: Math.max(0, parent.width * (Mpris ? Mpris.progress : 0.0))
                        radius: 1.5
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: "#74C7EC" }
                            GradientStop { position: 1.0; color: "#89B4FA" }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        anchors.topMargin: -4
                        anchors.bottomMargin: -4
                        cursorShape: Qt.PointingHandCursor
                        onClicked: (mouse) => {
                            if (Mpris && parent.width > 0) {
                                var frac = Math.max(0.0, Math.min(1.0, mouse.x / parent.width))
                                Mpris.seek(frac)
                            }
                        }
                    }
                }

                // Timestamp Readout 1:27 / 3:22
                Text {
                    text: (Mpris ? Mpris.positionText : "0:00") + " / " + (Mpris ? Mpris.lengthText : "0:00")
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    color: "#89B4FA"
                }
            }
        }

        // ── CONTROLS & DYNAMIC EQUALIZER ─────────────────────────────────
        RowLayout {
            Layout.alignment: Qt.AlignVCenter
            spacing: 8

            // 4-Bar Dancing Visualizer
            Row {
                spacing: 2
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
                            NumberAnimation { from: 4 + index * 2; to: 14 - index; duration: 240 + index * 50; easing.type: Easing.InOutQuad }
                            NumberAnimation { from: 14 - index; to: 4 + index * 2; duration: 260 + index * 40; easing.type: Easing.InOutQuad }
                        }
                    }
                }
            }

            // Transport Buttons
            Row {
                spacing: 5
                Layout.alignment: Qt.AlignVCenter

                TransportButton {
                    size: 26
                    iconText: "⏮"
                    iconSize: 9
                    onActivated: if (Mpris) Mpris.previous()
                }

                TransportButton {
                    size: 30
                    iconText: (Mpris && Mpris.isPlaying) ? "❚❚" : "▶"
                    iconSize: (Mpris && Mpris.isPlaying) ? 10 : 11
                    isPrimary: true
                    onActivated: if (Mpris) Mpris.playPause()
                }

                TransportButton {
                    size: 26
                    iconText: "⏭"
                    iconSize: 9
                    onActivated: if (Mpris) Mpris.next()
                }
            }
        }
    }
}
