import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import ArchTitan.Media 1.0

Window {
    id: rootWindow
    title: "titan-media-hud"
    width: 480
    height: 84
    
    // Window flags for transparent, frameless Wayland overlay
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.BypassWindowManagerHint | Qt.Popup | Qt.Tool
    color: "transparent"
    visible: (Mpris && Mpris.hasMedia) || exitAnim.running

    // Position centered horizontally just below Waybar
    x: Screen.width > 0 ? Math.round((Screen.width - width) / 2) : 720
    y: 46

    // Drop Shadow Glow Behind the Pill
    Rectangle {
        anchors.fill: pillContainer
        anchors.margins: -4
        radius: pillContainer.radius + 4
        color: "#50000000"
        z: -2
    }

    // Dynamic Island / macOS Pill Container
    Rectangle {
        id: pillContainer
        anchors.fill: parent
        radius: 26
        color: "#F00A0A0E" // Ultra-deep Obsidian Glass (94% opacity)
        border.color: "#2EFFFFFF" // 1px Apple Specular Frosted Rim
        border.width: 1
        clip: true

        // Inner Top Specular Liquid Light
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            height: 1
            color: "#30FFFFFF"
            radius: 1
        }

        // Main Horizontal Content
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 16
            anchors.topMargin: 10
            anchors.bottomMargin: 10
            spacing: 12

            // ─── 1. ALBUM ARTWORK ──────────────────────────────────────────────
            Rectangle {
                id: artWrapper
                Layout.preferredWidth: 54
                Layout.preferredHeight: 54
                Layout.alignment: Qt.AlignVCenter
                radius: 15
                color: "#16161C"
                border.color: (Mpris && Mpris.isPlaying) ? "#4038BDF8" : "#20FFFFFF"
                border.width: 1
                clip: true

                // Fallback macOS Music Icon
                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#2A2A36" }
                        GradientStop { position: 1.0; color: "#14141A" }
                    }
                    visible: !artImg.visible || artImg.status !== Image.Ready

                    Text {
                        anchors.centerIn: parent
                        text: "♫"
                        font.pixelSize: 22
                        color: "#94A3B8"
                    }
                }

                Image {
                    id: artImg
                    anchors.fill: parent
                    source: Mpris ? Mpris.artUrl : ""
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    mipmap: true
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

                // Breathing Glow on Playing
                Rectangle {
                    anchors.fill: parent
                    radius: parent.radius
                    color: "transparent"
                    border.color: "#8038BDF8"
                    border.width: 1.5
                    visible: Mpris && Mpris.isPlaying
                    opacity: 0.6

                    SequentialAnimation on opacity {
                        running: Mpris && Mpris.isPlaying
                        loops: Animation.Infinite
                        NumberAnimation { from: 0.2; to: 0.8; duration: 1200; easing.type: Easing.InOutQuad }
                        NumberAnimation { from: 0.8; to: 0.2; duration: 1200; easing.type: Easing.InOutQuad }
                    }
                }
            }

            // ─── 2. TRACK METADATA & MICRO-SCRUBBER ────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 2

                // Header Row: App Pill + Track Title + Live Waveform
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    // Source Badge Pill (e.g. SPOTIFY, BRAVE, MPV)
                    Rectangle {
                        height: 15
                        width: sourceBadgeText.implicitWidth + 8
                        radius: 7
                        color: "#2538BDF8"
                        border.color: "#4038BDF8"
                        border.width: 1

                        Text {
                            id: sourceBadgeText
                            anchors.centerIn: parent
                            text: (Mpris && Mpris.playerName ? Mpris.playerName : "NOW PLAYING").toUpperCase()
                            font.family: "JetBrainsMono Nerd Font"
                            font.pixelSize: 8
                            font.weight: Font.Bold
                            font.letterSpacing: 0.6
                            color: "#38BDF8"
                        }
                    }

                    // Live Animated Equalizer Bars (Apple Dynamic Island Style)
                    Row {
                        spacing: 2
                        Layout.alignment: Qt.AlignVCenter
                        visible: Mpris && Mpris.isPlaying

                        Rectangle {
                            width: 2.5; radius: 1.25; color: "#38BDF8"
                            height: 10
                            SequentialAnimation on height {
                                running: Mpris && Mpris.isPlaying
                                loops: Animation.Infinite
                                NumberAnimation { from: 4; to: 12; duration: 320; easing.type: Easing.InOutSine }
                                NumberAnimation { from: 12; to: 4; duration: 380; easing.type: Easing.InOutSine }
                            }
                        }
                        Rectangle {
                            width: 2.5; radius: 1.25; color: "#38BDF8"
                            height: 6
                            SequentialAnimation on height {
                                running: Mpris && Mpris.isPlaying
                                loops: Animation.Infinite
                                NumberAnimation { from: 12; to: 5; duration: 400; easing.type: Easing.InOutSine }
                                NumberAnimation { from: 5; to: 12; duration: 300; easing.type: Easing.InOutSine }
                            }
                        }
                        Rectangle {
                            width: 2.5; radius: 1.25; color: "#38BDF8"
                            height: 8
                            SequentialAnimation on height {
                                running: Mpris && Mpris.isPlaying
                                loops: Animation.Infinite
                                NumberAnimation { from: 3; to: 10; duration: 350; easing.type: Easing.InOutSine }
                                NumberAnimation { from: 10; to: 3; duration: 420; easing.type: Easing.InOutSine }
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Elapsed / Total Duration Label
                    Text {
                        text: (Mpris ? Mpris.positionText : "0:00") + " / " + (Mpris ? Mpris.lengthText : "0:00")
                        font.family: "JetBrainsMono Nerd Font"
                        font.pixelSize: 9
                        color: "#71717A"
                        visible: Mpris ? Mpris.length > 0 : false
                    }
                }

                // Track Title (Crisp White SF-Style Typography)
                Text {
                    Layout.fillWidth: true
                    text: Mpris ? Mpris.title : "No Media"
                    font.family: "Inter, -apple-system, BlinkMacSystemFont, sans-serif"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    color: "#FFFFFF"
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                // Artist / Subtitle
                Text {
                    Layout.fillWidth: true
                    text: Mpris ? Mpris.artist : "ArchTitan Desktop"
                    font.family: "Inter, -apple-system, BlinkMacSystemFont, sans-serif"
                    font.pixelSize: 11
                    color: "#A1A1AA"
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }

                Item { Layout.preferredHeight: 1 }

                // Interactive Apple Scrubber Bar
                Rectangle {
                    id: progressTrack
                    Layout.fillWidth: true
                    Layout.preferredHeight: seekMouseArea.containsMouse ? 5 : 3.5
                    radius: height / 2
                    color: "#27272A"

                    Behavior on Layout.preferredHeight {
                        NumberAnimation { duration: 120 }
                    }

                    // Progress Fill (Cyan Glow Gradient)
                    Rectangle {
                        height: parent.height
                        width: Math.max(0, Math.min(parent.width, parent.width * (Mpris ? Mpris.progress : 0)))
                        radius: parent.radius
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: "#0284C7" }
                            GradientStop { position: 1.0; color: "#38BDF8" }
                        }

                        // Scrubber Thumb Pill
                        Rectangle {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            width: seekMouseArea.containsMouse ? 8 : 0
                            height: seekMouseArea.containsMouse ? 8 : 0
                            radius: 4
                            color: "#FFFFFF"
                            visible: seekMouseArea.containsMouse || seekMouseArea.pressed

                            Behavior on width { NumberAnimation { duration: 100 } }
                            Behavior on height { NumberAnimation { duration: 100 } }
                        }
                    }

                    MouseArea {
                        id: seekMouseArea
                        anchors.fill: parent
                        anchors.margins: -6
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: (mouse) => {
                            if (!Mpris) return;
                            var frac = Math.max(0.0, Math.min(1.0, mouse.x / progressTrack.width));
                            Mpris.seek(frac);
                        }
                    }
                }
            }

            // ─── 3. APPLE STYLE MEDIA CONTROLS ────────────────────────────────
            RowLayout {
                Layout.alignment: Qt.AlignVCenter
                spacing: 8

                // Previous Track Button
                Rectangle {
                    width: 26
                    height: 26
                    radius: 13
                    color: prevArea.containsMouse ? "#20FFFFFF" : "#10FFFFFF"
                    scale: prevArea.pressed ? 0.88 : (prevArea.containsMouse ? 1.06 : 1.0)

                    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
                    Behavior on color { ColorAnimation { duration: 120 } }

                    Text {
                        anchors.centerIn: parent
                        text: "⏮"
                        font.pixelSize: 11
                        color: prevArea.containsMouse ? "#FFFFFF" : "#A1A1AA"
                    }

                    MouseArea {
                        id: prevArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (Mpris) Mpris.previous()
                    }
                }

                // Play / Pause Button (Apple White Primary Pill)
                Rectangle {
                    width: 32
                    height: 32
                    radius: 16
                    color: (Mpris && Mpris.isPlaying) ? "#FFFFFF" : "#38BDF8"
                    scale: playArea.pressed ? 0.88 : (playArea.containsMouse ? 1.08 : 1.0)

                    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
                    Behavior on color { ColorAnimation { duration: 150 } }

                    Text {
                        anchors.centerIn: parent
                        anchors.horizontalCenterOffset: (Mpris && Mpris.isPlaying) ? 0 : 1
                        text: (Mpris && Mpris.isPlaying) ? "❚❚" : "▶"
                        font.pixelSize: (Mpris && Mpris.isPlaying) ? 11 : 12
                        font.weight: Font.Bold
                        color: (Mpris && Mpris.isPlaying) ? "#0A0A0E" : "#0A0A0E"
                    }

                    MouseArea {
                        id: playArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (Mpris) Mpris.playPause()
                    }
                }

                // Next Track Button
                Rectangle {
                    width: 26
                    height: 26
                    radius: 13
                    color: nextArea.containsMouse ? "#20FFFFFF" : "#10FFFFFF"
                    scale: nextArea.pressed ? 0.88 : (nextArea.containsMouse ? 1.06 : 1.0)

                    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
                    Behavior on color { ColorAnimation { duration: 120 } }

                    Text {
                        anchors.centerIn: parent
                        text: "⏭"
                        font.pixelSize: 11
                        color: nextArea.containsMouse ? "#FFFFFF" : "#A1A1AA"
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

        // ─── 4. FLUID MACOS SPRING ANIMATIONS ─────────────────────────────────
        transform: [
            Translate { id: trans; y: -36 },
            Scale { id: cardScale; origin.x: pillContainer.width / 2; origin.y: 0; xScale: 0.9; yScale: 0.9 }
        ]
        opacity: 0.0

        ParallelAnimation {
            id: enterAnim
            NumberAnimation { target: trans; property: "y"; from: -36; to: 0; duration: 260; easing.type: Easing.OutBack; easing.overshoot: 1.1 }
            NumberAnimation { target: cardScale; property: "xScale"; from: 0.9; to: 1.0; duration: 240; easing.type: Easing.OutCubic }
            NumberAnimation { target: cardScale; property: "yScale"; from: 0.9; to: 1.0; duration: 240; easing.type: Easing.OutCubic }
            NumberAnimation { target: pillContainer; property: "opacity"; from: 0.0; to: 1.0; duration: 200; easing.type: Easing.OutQuad }
        }

        ParallelAnimation {
            id: exitAnim
            NumberAnimation { target: trans; property: "y"; from: 0; to: -36; duration: 200; easing.type: Easing.InBack }
            NumberAnimation { target: cardScale; property: "xScale"; from: 1.0; to: 0.9; duration: 180; easing.type: Easing.InCubic }
            NumberAnimation { target: cardScale; property: "yScale"; from: 1.0; to: 0.9; duration: 180; easing.type: Easing.InCubic }
            NumberAnimation { target: pillContainer; property: "opacity"; from: 1.0; to: 0.0; duration: 160; easing.type: Easing.InQuad }
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
