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
                active: typeof networkCtrl !== "undefined" && networkCtrl && networkCtrl.wifiEnabled
                iconText: "📶"
                titleText: "Internet"
                statusText: (typeof networkCtrl !== "undefined" && networkCtrl) ? networkCtrl.ssid : "moto g24 power"
                onClicked: if (typeof networkCtrl !== "undefined" && networkCtrl) networkCtrl.toggleWifi()
            }

            LargeToggleTile {
                active: typeof bluetoothCtrl !== "undefined" && bluetoothCtrl && bluetoothCtrl.powered
                iconText: "ᛒ"
                titleText: "Bluetooth"
                statusText: (typeof bluetoothCtrl !== "undefined" && bluetoothCtrl) ? bluetoothCtrl.statusText : "Not connected"
                onClicked: if (typeof bluetoothCtrl !== "undefined" && bluetoothCtrl) bluetoothCtrl.toggleBluetooth()
            }

            // Caffeine Mini Button
            Rectangle {
                width: 46; height: 46; radius: 23
                color: (typeof systemCtrl !== "undefined" && systemCtrl && systemCtrl.caffeineActive) ? "#38BDF8" : "#1F2330"
                border.color: (typeof systemCtrl !== "undefined" && systemCtrl && systemCtrl.caffeineActive) ? "#38BDF8" : "#20FFFFFF"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "☕"
                    font.pixelSize: 16
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: if (typeof systemCtrl !== "undefined" && systemCtrl) systemCtrl.caffeineActive = !systemCtrl.caffeineActive
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
                color: (typeof audioCtrl !== "undefined" && audioCtrl && !audioCtrl.sourceMuted) ? "#38BDF8" : "#1F2330"
                border.color: (typeof audioCtrl !== "undefined" && audioCtrl && !audioCtrl.sourceMuted) ? "#38BDF8" : "#20FFFFFF"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: (typeof audioCtrl !== "undefined" && audioCtrl && audioCtrl.sourceMuted) ? "🎙✕" : "🎙"
                    font.pixelSize: 14
                    color: (typeof audioCtrl !== "undefined" && audioCtrl && !audioCtrl.sourceMuted) ? "#0A1322" : "#FFFFFF"
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: if (typeof audioCtrl !== "undefined" && audioCtrl) audioCtrl.toggleSourceMute()
                }
            }

            LargeToggleTile {
                active: typeof audioCtrl !== "undefined" && audioCtrl && !audioCtrl.sinkMuted
                iconText: (typeof audioCtrl !== "undefined" && audioCtrl && audioCtrl.sinkMuted) ? "🔇" : "🔊"
                titleText: "Audio output"
                statusText: (typeof audioCtrl !== "undefined" && audioCtrl) ? audioCtrl.statusText : "Muted"
                onClicked: if (typeof audioCtrl !== "undefined" && audioCtrl) audioCtrl.toggleSinkMute()
            }

            LargeToggleTile {
                active: typeof nightLightCtrl !== "undefined" && nightLightCtrl && nightLightCtrl.active
                iconText: "🌙"
                titleText: "Night Light"
                statusText: (typeof nightLightCtrl !== "undefined" && nightLightCtrl) ? nightLightCtrl.statusText : "Inactive"
                onClicked: if (typeof nightLightCtrl !== "undefined" && nightLightCtrl) nightLightCtrl.toggleNightLight()
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
                color: (typeof systemCtrl !== "undefined" && systemCtrl && systemCtrl.themeDark) ? "#38BDF8" : "#1F2330"
                Text { anchors.centerIn: parent; text: "◑"; font.pixelSize: 18; color: (typeof systemCtrl !== "undefined" && systemCtrl && systemCtrl.themeDark) ? "#0A1322" : "#FFFFFF" }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (typeof systemCtrl !== "undefined" && systemCtrl) systemCtrl.themeDark = !systemCtrl.themeDark }
            }

            // 2. Audio Visualizer
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                color: (typeof systemCtrl !== "undefined" && systemCtrl && systemCtrl.visualizerActive) ? "#38BDF8" : "#1F2330"
                Text { anchors.centerIn: parent; text: "ılı"; font.pixelSize: 18; font.bold: true; color: (typeof systemCtrl !== "undefined" && systemCtrl && systemCtrl.visualizerActive) ? "#0A1322" : "#FFFFFF" }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (typeof systemCtrl !== "undefined" && systemCtrl) systemCtrl.visualizerActive = !systemCtrl.visualizerActive }
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
                color: (typeof systemCtrl !== "undefined" && systemCtrl && systemCtrl.vpnActive) ? "#38BDF8" : "#1F2330"
                Text { anchors.centerIn: parent; text: "☁🔒"; font.pixelSize: 14; color: (typeof systemCtrl !== "undefined" && systemCtrl && systemCtrl.vpnActive) ? "#0A1322" : "#FFFFFF" }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (typeof systemCtrl !== "undefined" && systemCtrl) systemCtrl.vpnActive = !systemCtrl.vpnActive }
            }

            // 5. Game Mode
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                color: (typeof systemCtrl !== "undefined" && systemCtrl && systemCtrl.gameModeActive) ? "#38BDF8" : "#1F2330"
                Text { anchors.centerIn: parent; text: "🎮"; font.pixelSize: 16 }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (typeof systemCtrl !== "undefined" && systemCtrl) systemCtrl.gameModeActive = !systemCtrl.gameModeActive }
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
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (typeof systemCtrl !== "undefined" && systemCtrl) systemCtrl.takeScreenshot() }
            }

            // 2. Color Picker
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                color: "#1F2330"
                Text { anchors.centerIn: parent; text: "💉"; font.pixelSize: 15 }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (typeof systemCtrl !== "undefined" && systemCtrl) systemCtrl.pickColor() }
            }

            // 3. Virtual Keyboard
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                color: "#1F2330"
                Text { anchors.centerIn: parent; text: "⌨"; font.pixelSize: 17; color: "#FFFFFF" }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (typeof systemCtrl !== "undefined" && systemCtrl) systemCtrl.toggleVirtualKeyboard() }
            }

            // 4. Notifications Bell
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                color: (typeof notifServer !== "undefined" && notifServer && !notifServer.dndActive) ? "#38BDF8" : "#1F2330"
                Text { anchors.centerIn: parent; text: "🔔"; font.pixelSize: 15; color: (typeof notifServer !== "undefined" && notifServer && !notifServer.dndActive) ? "#0A1322" : "#FFFFFF" }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (typeof notifServer !== "undefined" && notifServer) notifServer.toggleDnd() }
            }

            // 5. Music Widget
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 22
                color: "#1F2330"
                Text { anchors.centerIn: parent; text: "🎵"; font.pixelSize: 16 }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (typeof systemCtrl !== "undefined" && systemCtrl) systemCtrl.openMediaWidget() }
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
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: if (typeof audioCtrl !== "undefined" && audioCtrl) audioCtrl.toggleSourceMute() }
            }
            Item { Layout.fillWidth: true }
        }
    }
}
