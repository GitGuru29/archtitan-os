import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import ArchTitan.Media 1.0

Window {
    id: rootWindow
    title: "titan-media-hud"
    width: 440
    height: 110
    
    // Window flags for transparent, frameless Wayland overlay
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.BypassWindowManagerHint | Qt.Tool
    color: "transparent"
    visible: (Mpris && Mpris.hasMedia) || exitAnim.running

    // Position centered horizontally just below Waybar
    x: Screen.width > 0 ? Math.round((Screen.width - width) / 2) : 740
    y: 52

    // Main Card Container with Slide & Fade Animation
    Rectangle {
        id: card
        anchors.fill: parent
        radius: 10
        color: "#E611111B" // 90% opacity Catppuccin Crust
        border.color: "#2E89B4FA" // Subtle 1px blue accent border
        border.width: 1

        // Internal glass highlight
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 1
            color: "#1AFFFFFF"
            radius: 10
        }

        // Drop shadow feel
        Rectangle {
            anchors.fill: parent
            anchors.margins: -1
            radius: 11
            color: "transparent"
            border.color: "#0D000000"
            border.width: 1
            z: -1
        }

        // Layout
        RowLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12

            // Album Artwork Container
            Rectangle {
                Layout.preferredWidth: 84
                Layout.preferredHeight: 84
                Layout.alignment: Qt.AlignVCenter
                radius: 6
                color: "#181825"
                border.color: "#2589B4FA"
                border.width: 1
                clip: true

                // Fallback glyph when no artwork is available
                Text {
                    anchors.centerIn: parent
                    text: "◈"
                    font.pixelSize: 28
                    color: "#585B70"
                    visible: !artImg.visible || artImg.status !== Image.Ready
                }

                Image {
                    id: artImg
                    anchors.fill: parent
                    source: Mpris ? Mpris.artUrl : ""
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    visible: status === Image.Ready
                    opacity: 1.0

                    Behavior on source {
                        SequentialAnimation {
                            NumberAnimation { target: artImg; property: "opacity"; to: 0.0; duration: 100 }
                            PropertyAction { target: artImg; property: "source" }
                            NumberAnimation { target: artImg; property: "opacity"; to: 1.0; duration: 150 }
                        }
                    }
                }
            }

            // Track Information & Controls
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 3

                // Top Header Row: Source Badge + Status
                RowLayout {
                    Layout.fillWidth: true

                    // Source Badge (e.g. SPOTIFY, FIREFOX, VLC)
                    Rectangle {
                        height: 16
                        width: sourceText.implicitWidth + 10
                        radius: 3
                        color: "#2089B4FA"
                        border.color: "#4089B4FA"
                        border.width: 1

                        Text {
                            id: sourceText
                            anchors.centerIn: parent
                            text: (Mpris && Mpris.playerName ? Mpris.playerName : "MEDIA").toUpperCase()
                            font.family: "JetBrainsMono Nerd Font"
                            font.pixelSize: 9
                            font.weight: Font.Bold
                            font.letterSpacing: 0.8
                            color: "#89B4FA"
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Time Display (Elapsed / Total)
                    Text {
                        text: (Mpris ? Mpris.positionText : "0:00") + " / " + (Mpris ? Mpris.lengthText : "0:00")
                        font.family: "JetBrainsMono Nerd Font"
                        font.pixelSize: 10
                        color: "#6C7086"
                        visible: Mpris ? Mpris.length > 0 : false
                    }
                }

                // Track Title
                Text {
                    Layout.fillWidth: true
                    text: Mpris ? Mpris.title : "No Media"
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    color: "#CDD6F4"
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                // Artist
                Text {
                    Layout.fillWidth: true
                    text: Mpris ? Mpris.artist : "ArchTitan Desktop"
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 11
                    color: "#A6ADC8"
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                Item { Layout.preferredHeight: 1 }

                // Interactive Progress Bar
                Rectangle {
                    id: progressTrack
                    Layout.fillWidth: true
                    Layout.preferredHeight: 4
                    radius: 2
                    color: "#313244"

                    Rectangle {
                        height: parent.height
                        width: Math.max(0, Math.min(parent.width, parent.width * (Mpris ? Mpris.progress : 0)))
                        radius: 2
                        color: "#89B4FA"

                        Rectangle {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            width: 8
                            height: 8
                            radius: 4
                            color: "#CDD6F4"
                            visible: seekMouseArea.containsMouse || seekMouseArea.pressed
                        }
                    }

                    MouseArea {
                        id: seekMouseArea
                        anchors.fill: parent
                        anchors.margins: -4
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: (mouse) => {
                            if (!Mpris) return;
                            var frac = Math.max(0.0, Math.min(1.0, mouse.x / progressTrack.width));
                            Mpris.seek(frac);
                        }
                    }
                }

                // Playback Controls Row
                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 16

                    // Previous Button
                    Rectangle {
                        width: 24
                        height: 24
                        radius: 4
                        color: prevArea.containsMouse ? "#2089B4FA" : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "⏮"
                            font.pixelSize: 12
                            color: prevArea.containsMouse ? "#89B4FA" : "#A6ADC8"
                        }

                        MouseArea {
                            id: prevArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (Mpris) Mpris.previous()
                        }
                    }

                    // Play / Pause Button
                    Rectangle {
                        width: 26
                        height: 26
                        radius: 13
                        color: playArea.containsMouse ? "#3089B4FA" : "#1A89B4FA"
                        border.color: "#4089B4FA"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            anchors.horizontalCenterOffset: (Mpris && Mpris.isPlaying) ? 0 : 1
                            text: (Mpris && Mpris.isPlaying) ? "❚❚" : "▶"
                            font.pixelSize: 11
                            color: "#89B4FA"
                        }

                        MouseArea {
                            id: playArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (Mpris) Mpris.playPause()
                        }
                    }

                    // Next Button
                    Rectangle {
                        width: 24
                        height: 24
                        radius: 4
                        color: nextArea.containsMouse ? "#2089B4FA" : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "⏭"
                            font.pixelSize: 12
                            color: nextArea.containsMouse ? "#89B4FA" : "#A6ADC8"
                        }

                        MouseArea {
                            id: nextArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (Mpris) Mpris.next()
                        }
                    }
                }
            }
        }

        // Slide & Fade Animations
        transform: Translate { id: trans; y: -30 }
        opacity: 0.0

        ParallelAnimation {
            id: enterAnim
            NumberAnimation { target: trans; property: "y"; from: -30; to: 0; duration: 220; easing.type: Easing.OutCubic }
            NumberAnimation { target: card; property: "opacity"; from: 0.0; to: 1.0; duration: 200; easing.type: Easing.OutQuad }
        }

        ParallelAnimation {
            id: exitAnim
            NumberAnimation { target: trans; property: "y"; from: 0; to: -30; duration: 180; easing.type: Easing.InCubic }
            NumberAnimation { target: card; property: "opacity"; from: 1.0; to: 0.0; duration: 160; easing.type: Easing.InQuad }
        }

        Connections {
            target: Mpris
            function onMediaStateChanged() {
                if (Mpris.hasMedia) {
                    exitAnim.stop();
                    enterAnim.start();
                } else {
                    enterAnim.stop();
                    exitAnim.start();
                }
            }
        }

        Component.onCompleted: {
            if (Mpris && Mpris.hasMedia) {
                enterAnim.start();
            }
        }
    }
}
