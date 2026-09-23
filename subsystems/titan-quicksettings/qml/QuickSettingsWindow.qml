import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: rootWindow
    title: "titan-quicksettings"
    width: 420
    height: Screen.desktopAvailableHeight > 0 ? Math.min(Screen.desktopAvailableHeight - 24, 980) : 920

    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.BypassWindowManagerHint | Qt.Tool
    color: "transparent"

    x: Screen.width > 0 ? Screen.width - width - 12 : 1480
    y: 12

    property bool drawerOpen: false
    property bool drawerVisible: false

    visible: drawerVisible

    function openDrawer() {
        drawerVisible = true;
        drawerOpen = true;
        rootWindow.requestActivate();
    }

    function closeDrawer() {
        drawerOpen = false;
        closeTimer.start();
    }

    function toggleDrawer() {
        if (drawerOpen) {
            closeDrawer();
        } else {
            openDrawer();
        }
    }

    Timer {
        id: closeTimer
        interval: 280
        onTriggered: {
            if (!drawerOpen) {
                drawerVisible = false;
            }
        }
    }

    Item {
        anchors.fill: parent
        focus: true
        Keys.onEscapePressed: closeDrawer()
    }

    // Main Drawer Card Container with Slide Animation
    Rectangle {
        id: card
        width: parent.width
        height: parent.height
        radius: 22
        color: "#F011141D" // 94% opacity deep obsidian
        border.color: "#2E38BDF8" // Subtle 1px cyan border
        border.width: 1
        clip: true

        // Slide animation using overshot easing
        x: drawerOpen ? 0 : width + 30
        Behavior on x {
            NumberAnimation {
                duration: 260
                easing.type: Easing.OutBack
                easing.overshoot: 1.05
            }
        }

        // Top Subtle Glass Highlight
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 1
            color: "#25FFFFFF"
            radius: 22
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            // ════════════════════════════════════════════════════════════════════
            // 1. TOP HEADER BAR
            // ════════════════════════════════════════════════════════════════════
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 38
                spacing: 8

                // ArchTitan Logo + Uptime Badge
                Rectangle {
                    Layout.preferredHeight: 34
                    Layout.preferredWidth: 106
                    radius: 17
                    color: "#1C202C"
                    border.color: "#20FFFFFF"
                    border.width: 1

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 7

                        // Stylized Arch/Titan triangle delta icon
                        Canvas {
                            Layout.preferredWidth: 16
                            Layout.preferredHeight: 16
                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.clearRect(0, 0, width, height);
                                ctx.beginPath();
                                ctx.moveTo(width * 0.5, 2);
                                ctx.lineTo(width - 2, height - 2);
                                ctx.lineTo(width * 0.65, height - 2);
                                ctx.lineTo(width * 0.5, height * 0.52);
                                ctx.lineTo(width * 0.35, height - 2);
                                ctx.lineTo(2, height - 2);
                                ctx.closePath();
                                ctx.fillStyle = "#38BDF8";
                                ctx.fill();
                            }
                        }

                        Text {
                            text: systemCtrl ? systemCtrl.uptime : "Up 25m"
                            font.family: "Outfit, Inter, sans-serif"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            color: "#FFFFFF"
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Action Buttons: Edit, Refresh, Settings, Power
                RowLayout {
                    spacing: 6

                    // Edit Button
                    Rectangle {
                        width: 32; height: 32; radius: 16
                        color: editMa.containsMouse ? "#2A3042" : "#1A1D27"
                        Text { anchors.centerIn: parent; text: "✎"; color: "#94A3B8"; font.pixelSize: 14 }
                        MouseArea { id: editMa; anchors.fill: parent; hoverEnabled: true }
                    }

                    // Refresh Button
                    Rectangle {
                        width: 32; height: 32; radius: 16
                        color: refreshMa.containsMouse ? "#2A3042" : "#1A1D27"
                        Text { anchors.centerIn: parent; text: "↻"; color: "#94A3B8"; font.pixelSize: 15 }
                        MouseArea {
                            id: refreshMa; anchors.fill: parent; hoverEnabled: true
                            onClicked: {
                                if (systemCtrl) systemCtrl.refresh();
                                if (networkCtrl) networkCtrl.refresh();
                                if (bluetoothCtrl) bluetoothCtrl.refresh();
                                if (audioCtrl) audioCtrl.refresh();
                            }
                        }
                    }

                    // Settings Button
                    Rectangle {
                        width: 32; height: 32; radius: 16
                        color: setMa.containsMouse ? "#2A3042" : "#1A1D27"
                        Text { anchors.centerIn: parent; text: "⚙"; color: "#94A3B8"; font.pixelSize: 15 }
                        MouseArea {
                            id: setMa; anchors.fill: parent; hoverEnabled: true
                            onClicked: if (systemCtrl) systemCtrl.openSettings()
                        }
                    }

                    // Power Button
                    Rectangle {
                        width: 32; height: 32; radius: 16
                        color: pwrMa.containsMouse ? "#3A2024" : "#1A1D27"
                        Text { anchors.centerIn: parent; text: "⏻"; color: pwrMa.containsMouse ? "#F87171" : "#94A3B8"; font.pixelSize: 15 }
                        MouseArea {
                            id: pwrMa; anchors.fill: parent; hoverEnabled: true
                            onClicked: if (systemCtrl) systemCtrl.openPowerMenu()
                        }
                    }
                }
            }

            // ════════════════════════════════════════════════════════════════════
            // 2. QUICK SETTINGS CONTROLS (PILL TILES & SQUIRCLE GRID)
            // ════════════════════════════════════════════════════════════════════
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 270
                radius: 18
                color: "#141722"
                border.color: "#18FFFFFF"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 9

                    // ROW 1: Internet Pill, Bluetooth Pill, Caffeine
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 46
                        spacing: 8

                        // Internet Pill (Active Cyan)
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 46
                            radius: 23
                            color: (networkCtrl && networkCtrl.wifiEnabled) ? "#38BDF8" : "#1F2330"
                            border.color: (networkCtrl && networkCtrl.wifiEnabled) ? "#38BDF8" : "#28FFFFFF"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 10

                                // Wifi icon
                                Rectangle {
                                    width: 28; height: 28; radius: 14
                                    color: (networkCtrl && networkCtrl.wifiEnabled) ? "#200C1524" : "#2E3444"
                                    Text {
                                        anchors.centerIn: parent
                                        text: "📶"
                                        font.pixelSize: 13
                                    }
                                }

                                ColumnLayout {
                                    spacing: 0
                                    Text {
                                        text: "Internet"
                                        font.family: "Outfit, Inter, sans-serif"
                                        font.pixelSize: 13
                                        font.weight: Font.Bold
                                        color: (networkCtrl && networkCtrl.wifiEnabled) ? "#0A1322" : "#FFFFFF"
                                    }
                                    Text {
                                        text: networkCtrl ? networkCtrl.ssid : "moto g24 power"
                                        font.family: "Outfit, Inter, sans-serif"
                                        font.pixelSize: 11
                                        color: (networkCtrl && networkCtrl.wifiEnabled) ? "#1A324E" : "#94A3B8"
                                        elide: Text.ElideRight
                                        Layout.maximumWidth: 100
                                    }
                                }
                                Item { Layout.fillWidth: true }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (networkCtrl) networkCtrl.toggleWifi()
                            }
                        }

                        // Bluetooth Pill (Inactive Dark)
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 46
                            radius: 23
                            color: (bluetoothCtrl && bluetoothCtrl.powered) ? "#38BDF8" : "#1F2330"
                            border.color: (bluetoothCtrl && bluetoothCtrl.powered) ? "#38BDF8" : "#20FFFFFF"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 10

                                Rectangle {
                                    width: 28; height: 28; radius: 14
                                    color: (bluetoothCtrl && bluetoothCtrl.powered) ? "#200C1524" : "#2E3444"
                                    Text { anchors.centerIn: parent; text: "ᛒ"; font.pixelSize: 14; font.bold: true; color: (bluetoothCtrl && bluetoothCtrl.powered) ? "#0A1322" : "#FFFFFF" }
                                }

                                ColumnLayout {
                                    spacing: 0
                                    Text {
                                        text: "Bluetooth"
                                        font.family: "Outfit, Inter, sans-serif"
                                        font.pixelSize: 13
                                        font.weight: Font.Bold
                                        color: (bluetoothCtrl && bluetoothCtrl.powered) ? "#0A1322" : "#FFFFFF"
                                    }
                                    Text {
                                        text: bluetoothCtrl ? bluetoothCtrl.statusText : "Not connected"
                                        font.family: "Outfit, Inter, sans-serif"
                                        font.pixelSize: 11
                                        color: (bluetoothCtrl && bluetoothCtrl.powered) ? "#1A324E" : "#94A3B8"
                                        elide: Text.ElideRight
                                        Layout.maximumWidth: 100
                                    }
                                }
                                Item { Layout.fillWidth: true }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (bluetoothCtrl) bluetoothCtrl.toggleBluetooth()
                            }
                        }

                        // Caffeine Mini Button
                        Rectangle {
                            width: 46; height: 46; radius: 23
                            color: (systemCtrl && systemCtrl.caffeineActive) ? "#38BDF8" : "#1F2330"
                            border.color: (systemCtrl && systemCtrl.caffeineActive) ? "#38BDF8" : "#20FFFFFF"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: "☕"
                                font.pixelSize: 16
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (systemCtrl) systemCtrl.caffeineActive = !systemCtrl.caffeineActive
                            }
                        }
                    }

                    // ROW 2: Mic Mute, Audio Output, Night Light
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 46
                        spacing: 8

                        // Mic Muted Pill Button
                        Rectangle {
                            width: 46; height: 46; radius: 23
                            color: (audioCtrl && !audioCtrl.sourceMuted) ? "#38BDF8" : "#1F2330"
                            border.color: (audioCtrl && !audioCtrl.sourceMuted) ? "#38BDF8" : "#20FFFFFF"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: (audioCtrl && audioCtrl.sourceMuted) ? "🎙✕" : "🎙"
                                font.pixelSize: 14
                                color: (audioCtrl && !audioCtrl.sourceMuted) ? "#0A1322" : "#FFFFFF"
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (audioCtrl) audioCtrl.toggleSourceMute()
                            }
                        }

                        // Audio Output Pill
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 46
                            radius: 23
                            color: (audioCtrl && !audioCtrl.sinkMuted) ? "#38BDF8" : "#1F2330"
                            border.color: (audioCtrl && !audioCtrl.sinkMuted) ? "#38BDF8" : "#20FFFFFF"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 8

                                Text {
                                    text: (audioCtrl && audioCtrl.sinkMuted) ? "🔇" : "🔊"
                                    font.pixelSize: 15
                                }

                                ColumnLayout {
                                    spacing: 0
                                    Text {
                                        text: "Audio output"
                                        font.family: "Outfit, Inter, sans-serif"
                                        font.pixelSize: 12
                                        font.weight: Font.Bold
                                        color: (audioCtrl && !audioCtrl.sinkMuted) ? "#0A1322" : "#FFFFFF"
                                    }
                                    Text {
                                        text: audioCtrl ? audioCtrl.statusText : "Muted"
                                        font.family: "Outfit, Inter, sans-serif"
                                        font.pixelSize: 11
                                        color: (audioCtrl && !audioCtrl.sinkMuted) ? "#1A324E" : "#94A3B8"
                                    }
                                }
                                Item { Layout.fillWidth: true }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (audioCtrl) audioCtrl.toggleSinkMute()
                            }
                        }

                        // Night Light Pill
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 46
                            radius: 23
                            color: (nightLightCtrl && nightLightCtrl.active) ? "#38BDF8" : "#1F2330"
                            border.color: (nightLightCtrl && nightLightCtrl.active) ? "#38BDF8" : "#20FFFFFF"
                            border.width: 1

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 8

                                Text {
                                    text: "🌙"
                                    font.pixelSize: 14
                                }

                                ColumnLayout {
                                    spacing: 0
                                    Text {
                                        text: "Night Light"
                                        font.family: "Outfit, Inter, sans-serif"
                                        font.pixelSize: 12
                                        font.weight: Font.Bold
                                        color: (nightLightCtrl && nightLightCtrl.active) ? "#0A1322" : "#FFFFFF"
                                    }
                                    Text {
                                        text: nightLightCtrl ? nightLightCtrl.statusText : "Inactive"
                                        font.family: "Outfit, Inter, sans-serif"
                                        font.pixelSize: 11
                                        color: (nightLightCtrl && nightLightCtrl.active) ? "#1A324E" : "#94A3B8"
                                    }
                                }
                                Item { Layout.fillWidth: true }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (nightLightCtrl) nightLightCtrl.toggleNightLight()
                            }
                        }
                    }

                    // ROW 3 (5 SQUIRCLE ACTION BUTTONS)
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        spacing: 8

                        // 1. Contrast/Theme (Active cyan)
                        Rectangle {
                            Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                            color: (systemCtrl && systemCtrl.themeDark) ? "#38BDF8" : "#1F2330"
                            Text { anchors.centerIn: parent; text: "◑"; font.pixelSize: 18; color: (systemCtrl && systemCtrl.themeDark) ? "#0A1322" : "#FFFFFF" }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (systemCtrl) systemCtrl.themeDark = !systemCtrl.themeDark }
                        }

                        // 2. Audio Visualizer (Active cyan)
                        Rectangle {
                            Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                            color: (systemCtrl && systemCtrl.visualizerActive) ? "#38BDF8" : "#1F2330"
                            Text { anchors.centerIn: parent; text: "ılı"; font.pixelSize: 18; font.bold: true; color: (systemCtrl && systemCtrl.visualizerActive) ? "#0A1322" : "#FFFFFF" }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (systemCtrl) systemCtrl.visualizerActive = !systemCtrl.visualizerActive }
                        }

                        // 3. Waves / Ambient
                        Rectangle {
                            Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                            color: "#1F2330"
                            Text { anchors.centerIn: parent; text: "≈"; font.pixelSize: 20; color: "#FFFFFF" }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor }
                        }

                        // 4. Cloud Lock / VPN
                        Rectangle {
                            Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                            color: (systemCtrl && systemCtrl.vpnActive) ? "#38BDF8" : "#1F2330"
                            Text { anchors.centerIn: parent; text: "☁🔒"; font.pixelSize: 14; color: (systemCtrl && systemCtrl.vpnActive) ? "#0A1322" : "#FFFFFF" }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (systemCtrl) systemCtrl.vpnActive = !systemCtrl.vpnActive }
                        }

                        // 5. Game Mode / D-Pad
                        Rectangle {
                            Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                            color: (systemCtrl && systemCtrl.gameModeActive) ? "#38BDF8" : "#1F2330"
                            Text { anchors.centerIn: parent; text: "🎮"; font.pixelSize: 16 }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (systemCtrl) systemCtrl.gameModeActive = !systemCtrl.gameModeActive }
                        }
                    }

                    // ROW 4 (5 SQUIRCLE ACTION BUTTONS)
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        spacing: 8

                        // 1. Screenshot
                        Rectangle {
                            Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                            color: "#1F2330"
                            Text { anchors.centerIn: parent; text: "⛶"; font.pixelSize: 18; color: "#FFFFFF" }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (systemCtrl) systemCtrl.takeScreenshot() }
                        }

                        // 2. Color Picker (Eyedropper)
                        Rectangle {
                            Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                            color: "#1F2330"
                            Text { anchors.centerIn: parent; text: "💉"; font.pixelSize: 15 }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (systemCtrl) systemCtrl.pickColor() }
                        }

                        // 3. Virtual Keyboard
                        Rectangle {
                            Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                            color: "#1F2330"
                            Text { anchors.centerIn: parent; text: "⌨"; font.pixelSize: 17; color: "#FFFFFF" }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (systemCtrl) systemCtrl.toggleVirtualKeyboard() }
                        }

                        // 4. Notifications Bell (Active cyan)
                        Rectangle {
                            Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                            color: (notifServer && !notifServer.dndActive) ? "#38BDF8" : "#1F2330"
                            Text { anchors.centerIn: parent; text: "🔔"; font.pixelSize: 15; color: (notifServer && !notifServer.dndActive) ? "#0A1322" : "#FFFFFF" }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (notifServer) notifServer.toggleDnd() }
                        }

                        // 5. Music
                        Rectangle {
                            Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                            color: "#1F2330"
                            Text { anchors.centerIn: parent; text: "🎵"; font.pixelSize: 16 }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (systemCtrl) systemCtrl.openMediaWidget() }
                        }
                    }

                    // ROW 5: Bottom Mic Mute Button
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 38
                        spacing: 8

                        Rectangle {
                            width: 44; height: 38; radius: 19
                            color: "#1F2330"
                            border.color: "#20FFFFFF"
                            border.width: 1
                            Text { anchors.centerIn: parent; text: "🕬"; font.pixelSize: 16; color: "#FFFFFF" }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (audioCtrl) audioCtrl.toggleSourceMute() }
                        }
                        Item { Layout.fillWidth: true }
                    }
                }
            }

            // ════════════════════════════════════════════════════════════════════
            // 3. GROUPED NOTIFICATIONS LIST (SCROLLABLE)
            // ════════════════════════════════════════════════════════════════════
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ListView {
                    id: notifList
                    anchors.fill: parent
                    spacing: 8
                    clip: true
                    model: notifServer ? notifServer.notifications : []

                    delegate: Rectangle {
                        id: cardDelegate
                        width: notifList.width
                        height: modelData.expanded ? 120 : 66
                        radius: 14
                        color: cardMa.containsMouse ? "#1E2230" : "#171A24"
                        border.color: cardMa.containsMouse ? "#3038BDF8" : "#14FFFFFF"
                        border.width: 1

                        Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10

                            // Circular App Icon Badge
                            Rectangle {
                                width: 36; height: 36; radius: 18
                                color: (modelData.iconType === "antigravity") ? "#111420" : "#0284C7"
                                border.color: (modelData.iconType === "antigravity") ? "#38BDF8" : "transparent"
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: {
                                        if (modelData.iconType === "wifi") return "📶";
                                        if (modelData.iconType === "antigravity") return "▲";
                                        if (modelData.iconType === "terminal") return "💻";
                                        return "💬";
                                    }
                                    font.pixelSize: 14
                                    color: (modelData.iconType === "antigravity") ? "#38BDF8" : "#FFFFFF"
                                    font.bold: true
                                }
                            }

                            // Notification Content
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                // Top row: App Name, Date, Count Badge
                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: modelData.appName
                                        font.family: "Outfit, Inter, sans-serif"
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                        color: "#CBD5E1"
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: modelData.dateStr
                                        font.family: "Outfit, Inter, sans-serif"
                                        font.pixelSize: 11
                                        color: "#64748B"
                                    }

                                    // Expandable Count Badge
                                    Rectangle {
                                        height: 18
                                        width: countText.contentWidth + 14
                                        radius: 9
                                        color: "#242838"
                                        RowLayout {
                                            anchors.centerIn: parent
                                            spacing: 2
                                            Text {
                                                id: countText
                                                text: modelData.count + ""
                                                font.pixelSize: 10
                                                font.weight: Font.Bold
                                                color: "#94A3B8"
                                            }
                                            Text {
                                                text: modelData.expanded ? "▴" : "▾"
                                                font.pixelSize: 9
                                                color: "#94A3B8"
                                            }
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: if (notifServer) notifServer.toggleExpanded(index)
                                        }
                                    }
                                }

                                // Primary Text
                                Text {
                                    text: modelData.primaryText
                                    font.family: "Outfit, Inter, sans-serif"
                                    font.pixelSize: 12
                                    font.weight: Font.Bold
                                    color: "#FFFFFF"
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                // Secondary Text
                                Text {
                                    text: modelData.secondaryText
                                    font.family: "Outfit, Inter, sans-serif"
                                    font.pixelSize: 11
                                    color: "#94A3B8"
                                    elide: modelData.expanded ? Text.ElideNone : Text.ElideRight
                                    wrapMode: modelData.expanded ? Text.Wrap : Text.NoWrap
                                    Layout.fillWidth: true
                                }
                            }

                            // Dismiss button on hover
                            Rectangle {
                                width: 22; height: 22; radius: 11
                                color: dismissMa.containsMouse ? "#471D22" : "transparent"
                                visible: cardMa.containsMouse
                                Text { anchors.centerIn: parent; text: "×"; color: "#F87171"; font.pixelSize: 14; font.bold: true }
                                MouseArea {
                                    id: dismissMa
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: if (notifServer) notifServer.dismissAt(index)
                                }
                            }
                        }

                        MouseArea {
                            id: cardMa
                            anchors.fill: parent
                            hoverEnabled: true
                            propagateComposedEvents: true
                            onClicked: if (notifServer) notifServer.toggleExpanded(index)
                        }
                    }

                    // Empty State if all cleared
                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width - 40
                        height: 80
                        radius: 14
                        color: "#12151E"
                        visible: notifList.count === 0
                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 4
                            Text { text: "🔔"; font.pixelSize: 22; Layout.alignment: Qt.AlignHCenter }
                            Text { text: "No notifications — all caught up"; font.pixelSize: 12; color: "#64748B"; Layout.alignment: Qt.AlignHCenter }
                        }
                    }
                }
            }

            // ════════════════════════════════════════════════════════════════════
            // 4. NOTIFICATION FOOTER (DND, COUNT PILL, CLEAR ALL)
            // ════════════════════════════════════════════════════════════════════
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 38
                spacing: 8

                // DND Toggle
                Rectangle {
                    width: 36; height: 36; radius: 18
                    color: (notifServer && notifServer.dndActive) ? "#38BDF8" : "#1A1D27"
                    Text { anchors.centerIn: parent; text: "🔕"; font.pixelSize: 14 }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (notifServer) notifServer.toggleDnd()
                    }
                }

                // Total Notifications Pill
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    radius: 18
                    color: "#1A1D27"
                    Text {
                        anchors.centerIn: parent
                        text: notifServer ? (notifServer.totalCount + " notifications") : "8967 notifications"
                        font.family: "Outfit, Inter, sans-serif"
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        color: "#94A3B8"
                    }
                }

                // Clear All / Trash Button
                Rectangle {
                    width: 36; height: 36; radius: 18
                    color: trashMa.containsMouse ? "#3A2024" : "#1A1D27"
                    Text { anchors.centerIn: parent; text: "🗑"; font.pixelSize: 15; color: trashMa.containsMouse ? "#F87171" : "#94A3B8" }
                    MouseArea {
                        id: trashMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (notifServer) notifServer.clearAll()
                    }
                }
            }

            // ════════════════════════════════════════════════════════════════════
            // 5. BOTTOM CALENDAR & TASK DRAWER
            // ════════════════════════════════════════════════════════════════════
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                radius: 18
                color: "#161924"
                border.color: "#14FFFFFF"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    spacing: 10

                    Text {
                        text: "▲"
                        font.pixelSize: 10
                        color: "#64748B"
                    }

                    Text {
                        text: systemCtrl ? systemCtrl.currentDateString : "Thursday, September 17 • 0 tasks"
                        font.family: "Outfit, Inter, sans-serif"
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        color: "#CBD5E1"
                    }

                    Item { Layout.fillWidth: true }
                }
            }
        }
    }
}
