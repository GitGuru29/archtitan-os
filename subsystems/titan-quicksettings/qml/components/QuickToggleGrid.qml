import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: gridContainer
    Layout.fillWidth: true
    Layout.preferredHeight: 270
    radius: 18
    color: "#141722"
    border.color: "#18FFFFFF"
    border.width: 1

    required property var systemCtrl
    required property var networkCtrl
    required property var bluetoothCtrl
    required property var audioCtrl
    required property var nightLightCtrl
    required property var notifServer

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 9

        // ROW 1: Internet Pill, Bluetooth Pill, Caffeine
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            spacing: 8

            LargeToggleTile {
                active: gridContainer.networkCtrl && gridContainer.networkCtrl.wifiEnabled
                iconText: "📶"
                titleText: "Internet"
                statusText: gridContainer.networkCtrl ? gridContainer.networkCtrl.ssid : "moto g24 power"
                onClicked: if (gridContainer.networkCtrl) gridContainer.networkCtrl.toggleWifi()
            }

            LargeToggleTile {
                active: gridContainer.bluetoothCtrl && gridContainer.bluetoothCtrl.powered
                iconText: "ᛒ"
                titleText: "Bluetooth"
                statusText: gridContainer.bluetoothCtrl ? gridContainer.bluetoothCtrl.statusText : "Not connected"
                onClicked: if (gridContainer.bluetoothCtrl) gridContainer.bluetoothCtrl.toggleBluetooth()
            }

            // Caffeine Mini Button
            Rectangle {
                width: 46; height: 46; radius: 23
                color: (gridContainer.systemCtrl && gridContainer.systemCtrl.caffeineActive) ? "#38BDF8" : "#1F2330"
                border.color: (gridContainer.systemCtrl && gridContainer.systemCtrl.caffeineActive) ? "#38BDF8" : "#20FFFFFF"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "☕"
                    font.pixelSize: 16
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: if (gridContainer.systemCtrl) gridContainer.systemCtrl.caffeineActive = !gridContainer.systemCtrl.caffeineActive
                }
            }
        }

        // ROW 2: Mic Mute Button, Audio Output Pill, Night Light Pill
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            spacing: 8

            // Mic Muted Pill Button
            Rectangle {
                width: 46; height: 46; radius: 23
                color: (gridContainer.audioCtrl && !gridContainer.audioCtrl.sourceMuted) ? "#38BDF8" : "#1F2330"
                border.color: (gridContainer.audioCtrl && !gridContainer.audioCtrl.sourceMuted) ? "#38BDF8" : "#20FFFFFF"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: (gridContainer.audioCtrl && gridContainer.audioCtrl.sourceMuted) ? "🎙✕" : "🎙"
                    font.pixelSize: 14
                    color: (gridContainer.audioCtrl && !gridContainer.audioCtrl.sourceMuted) ? "#0A1322" : "#FFFFFF"
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: if (gridContainer.audioCtrl) gridContainer.audioCtrl.toggleSourceMute()
                }
            }

            LargeToggleTile {
                active: gridContainer.audioCtrl && !gridContainer.audioCtrl.sinkMuted
                iconText: (gridContainer.audioCtrl && gridContainer.audioCtrl.sinkMuted) ? "🔇" : "🔊"
                titleText: "Audio output"
                statusText: gridContainer.audioCtrl ? gridContainer.audioCtrl.statusText : "Muted"
                onClicked: if (gridContainer.audioCtrl) gridContainer.audioCtrl.toggleSinkMute()
            }

            LargeToggleTile {
                active: gridContainer.nightLightCtrl && gridContainer.nightLightCtrl.active
                iconText: "🌙"
                titleText: "Night Light"
                statusText: gridContainer.nightLightCtrl ? gridContainer.nightLightCtrl.statusText : "Inactive"
                onClicked: if (gridContainer.nightLightCtrl) gridContainer.nightLightCtrl.toggleNightLight()
            }
        }

        // ROW 3: 5 Squircle Action Buttons
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            spacing: 8

            // 1. Contrast/Theme
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                color: (gridContainer.systemCtrl && gridContainer.systemCtrl.themeDark) ? "#38BDF8" : "#1F2330"
                Text { anchors.centerIn: parent; text: "◑"; font.pixelSize: 18; color: (gridContainer.systemCtrl && gridContainer.systemCtrl.themeDark) ? "#0A1322" : "#FFFFFF" }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (gridContainer.systemCtrl) gridContainer.systemCtrl.themeDark = !gridContainer.systemCtrl.themeDark }
            }

            // 2. Audio Visualizer
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                color: (gridContainer.systemCtrl && gridContainer.systemCtrl.visualizerActive) ? "#38BDF8" : "#1F2330"
                Text { anchors.centerIn: parent; text: "ılı"; font.pixelSize: 18; font.bold: true; color: (gridContainer.systemCtrl && gridContainer.systemCtrl.visualizerActive) ? "#0A1322" : "#FFFFFF" }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (gridContainer.systemCtrl) gridContainer.systemCtrl.visualizerActive = !gridContainer.systemCtrl.visualizerActive }
            }

            // 3. Waves / Ambient
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                color: "#1F2330"
                Text { anchors.centerIn: parent; text: "≈"; font.pixelSize: 20; color: "#FFFFFF" }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor }
            }

            // 4. VPN Lock
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                color: (gridContainer.systemCtrl && gridContainer.systemCtrl.vpnActive) ? "#38BDF8" : "#1F2330"
                Text { anchors.centerIn: parent; text: "☁🔒"; font.pixelSize: 14; color: (gridContainer.systemCtrl && gridContainer.systemCtrl.vpnActive) ? "#0A1322" : "#FFFFFF" }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (gridContainer.systemCtrl) gridContainer.systemCtrl.vpnActive = !gridContainer.systemCtrl.vpnActive }
            }

            // 5. Game Mode
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                color: (gridContainer.systemCtrl && gridContainer.systemCtrl.gameModeActive) ? "#38BDF8" : "#1F2330"
                Text { anchors.centerIn: parent; text: "🎮"; font.pixelSize: 16 }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (gridContainer.systemCtrl) gridContainer.systemCtrl.gameModeActive = !gridContainer.systemCtrl.gameModeActive }
            }
        }

        // ROW 4: 5 Squircle Action Buttons
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            spacing: 8

            // 1. Screenshot
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                color: "#1F2330"
                Text { anchors.centerIn: parent; text: "⛶"; font.pixelSize: 18; color: "#FFFFFF" }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (gridContainer.systemCtrl) gridContainer.systemCtrl.takeScreenshot() }
            }

            // 2. Color Picker
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                color: "#1F2330"
                Text { anchors.centerIn: parent; text: "💉"; font.pixelSize: 15 }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (gridContainer.systemCtrl) gridContainer.systemCtrl.pickColor() }
            }

            // 3. Virtual Keyboard
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                color: "#1F2330"
                Text { anchors.centerIn: parent; text: "⌨"; font.pixelSize: 17; color: "#FFFFFF" }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (gridContainer.systemCtrl) gridContainer.systemCtrl.toggleVirtualKeyboard() }
            }

            // 4. Notifications Bell
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                color: (gridContainer.notifServer && !gridContainer.notifServer.dndActive) ? "#38BDF8" : "#1F2330"
                Text { anchors.centerIn: parent; text: "🔔"; font.pixelSize: 15; color: (gridContainer.notifServer && !gridContainer.notifServer.dndActive) ? "#0A1322" : "#FFFFFF" }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (gridContainer.notifServer) gridContainer.notifServer.toggleDnd() }
            }

            // 5. Music Widget
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                color: "#1F2330"
                Text { anchors.centerIn: parent; text: "🎵"; font.pixelSize: 16 }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (gridContainer.systemCtrl) gridContainer.systemCtrl.openMediaWidget() }
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
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (gridContainer.audioCtrl) gridContainer.audioCtrl.toggleSourceMute() }
            }
            Item { Layout.fillWidth: true }
        }
    }
}
