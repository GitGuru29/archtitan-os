import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import ArchTitanSettings

Item {
    id: pageRoot
    anchors.fill: parent

    property color textHigh: globalTextHigh
    property color textMid:  globalTextMid
    property color textLow:  globalTextLow
    property color accent:   SettingsBackend.accentColor
    property color green:    "#4CAF82"
    property color red:      "#E05C6A"
    property color orange:   "#D4853A"

    // Toast state
    property string toastText: ""
    property bool toastIsError: false

    function showToast(msg, isErr) {
        toastText = msg
        toastIsError = !!isErr
        toastTimer.restart()
    }

    Timer {
        id: toastTimer
        interval: 3500
        onTriggered: toastText = ""
    }

    // Password modal state
    property string targetSsid: ""
    property bool targetIsSecured: true
    property bool showPasswordPrompt: false
    property string passwordInput: ""
    property bool hidePasswordChars: true

    function openConnectDialog(ssid, security) {
        targetSsid = ssid
        targetIsSecured = (security !== "--" && security !== "" && security.indexOf("Open") === -1)
        passwordInput = ""
        hidePasswordChars = true

        if (!targetIsSecured || NetworkManager.savedNetworks.indexOf(ssid) !== -1) {
            NetworkManager.connectToNetwork(ssid, "")
        } else {
            showPasswordPrompt = true
        }
    }

    Connections {
        target: NetworkManager
        function onConnectionSuccess(ssid) {
            showPasswordPrompt = false
            showToast("Connected to " + ssid, false)
        }
        function onConnectionError(msg) {
            showToast(msg, true)
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: parent.width
            spacing: 0

            Item { height: 20 }

            // ── Header Bar ─────────────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 24; Layout.rightMargin: 24

                Text {
                    text: "Network"
                    font { pixelSize: 22; family: "Inter" }
                    font.weight: Font.Bold
                    color: pageRoot.textHigh
                }

                Item { Layout.fillWidth: true }

                RowLayout {
                    spacing: 10
                    Text {
                        text: "Wi-Fi"
                        font { pixelSize: 13; family: "Inter" }
                        font.weight: Font.Medium
                        color: pageRoot.textMid
                    }
                    TitanSwitch {
                        onColor: pageRoot.accent
                        checked: NetworkManager.wifiEnabled
                        onCheckedChanged: NetworkManager.wifiEnabled = checked
                    }
                }
            }

            Item { height: 20 }

            // ── Top Stats Bar ──────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 24; Layout.rightMargin: 24
                height: 76; radius: 10
                color: globalBg3
                border.width: 1; border.color: globalBorder1

                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    // SIGNAL
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        Layout.leftMargin: 16
                        spacing: 4

                        Text {
                            text: "SIGNAL"
                            font { pixelSize: 9; family: "Inter" }
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.2
                            color: pageRoot.textLow
                        }

                        RowLayout {
                            spacing: 2
                            Text {
                                text: NetworkManager.isConnected ? NetworkManager.signalStrength : "0"
                                font { pixelSize: 22; family: "Inter" }
                                font.weight: Font.Bold
                                color: NetworkManager.isConnected ? pageRoot.green : pageRoot.textMid
                            }
                            Text {
                                text: "%"
                                font { pixelSize: 12; family: "Inter" }
                                font.weight: Font.Bold
                                color: NetworkManager.isConnected ? pageRoot.green : pageRoot.textMid
                            }
                        }
                    }

                    Rectangle { width: 1; height: 44; color: globalBorder1 }

                    // LINK SPEED
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        Layout.leftMargin: 16
                        spacing: 4

                        Text {
                            text: "LINK SPEED"
                            font { pixelSize: 9; family: "Inter" }
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.2
                            color: pageRoot.textLow
                        }

                        RowLayout {
                            spacing: 4
                            Text {
                                text: NetworkManager.isConnected ? (NetworkManager.linkSpeed.length > 0 ? NetworkManager.linkSpeed.replace(/[^0-9]/g, '') : "---") : "---"
                                font { pixelSize: 22; family: "Inter" }
                                font.weight: Font.Bold
                                color: pageRoot.textHigh
                            }
                            Text {
                                text: "Mbit/s"
                                font { pixelSize: 12; family: "Inter" }
                                font.weight: Font.Medium
                                color: pageRoot.textMid
                            }
                        }
                    }

                    Rectangle { width: 1; height: 44; color: globalBorder1 }

                    // STATUS
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        Layout.leftMargin: 16
                        spacing: 4

                        Text {
                            text: "STATUS"
                            font { pixelSize: 9; family: "Inter" }
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.2
                            color: pageRoot.textLow
                        }

                        Text {
                            text: NetworkManager.ethernetConnected ? "Wired Link" : (NetworkManager.isConnected ? "Connected" : "Disconnected")
                            font { pixelSize: 15; family: "Inter" }
                            font.weight: Font.Bold
                            color: (NetworkManager.ethernetConnected || NetworkManager.isConnected) ? pageRoot.green : pageRoot.red
                        }
                    }

                    Rectangle { width: 1; height: 44; color: globalBorder1 }

                    // NETWORK
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        Layout.leftMargin: 16; Layout.rightMargin: 16
                        spacing: 4

                        Text {
                            text: "NETWORK"
                            font { pixelSize: 9; family: "Inter" }
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.2
                            color: pageRoot.textLow
                        }

                        Text {
                            text: NetworkManager.ethernetConnected ? "Ethernet (" + NetworkManager.ethernetInterface + ")" : (NetworkManager.isConnected ? NetworkManager.connectedSsid : "Not Connected")
                            font { pixelSize: 14; family: "Inter" }
                            font.weight: Font.Medium
                            color: pageRoot.textHigh
                            elide: Text.ElideRight
                        }
                    }
                }
            }

            Item { height: 16 }

            // ── 2-Column Main Section ──────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 24; Layout.rightMargin: 24
                spacing: 16
                Layout.alignment: Qt.AlignTop

                // Left Column (Speed Test & Available Networks)
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 600
                    spacing: 16
                    Layout.alignment: Qt.AlignTop

                    // SPEED TEST Card
                    SettingsCard {
                        Layout.fillWidth: true
                        title: "SPEED TEST"

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 16

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 16

                                // Circular Speed Gauge Badge
                                Rectangle {
                                    width: 72; height: 72; radius: 36
                                    color: globalBg4
                                    border.width: 2
                                    border.color: NetworkManager.isSpeedTestRunning ? pageRoot.green : globalBorder0

                                    ColumnLayout {
                                        anchors.centerIn: parent
                                        spacing: 2
                                        Text {
                                            text: NetworkManager.isSpeedTestRunning ? "RUNNING" : "IDLE"
                                            font { pixelSize: 9; family: "Inter" }
                                            font.weight: Font.Bold
                                            color: NetworkManager.isSpeedTestRunning ? pageRoot.green : pageRoot.textLow
                                            Layout.alignment: Qt.AlignHCenter
                                        }
                                    }
                                }

                                Text {
                                    text: "Checks real-time throughput and latency. Temporarily raises background activity."
                                    font { pixelSize: 12; family: "Inter" }
                                    color: pageRoot.textMid
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    lineHeight: 1.3
                                }
                            }

                            TitanButton {
                                text: NetworkManager.isSpeedTestRunning ? "Stop test" : "Start test"
                                primary: !NetworkManager.isSpeedTestRunning
                                width: 120
                                onClicked: NetworkManager.toggleSpeedTest()
                            }

                            Rectangle { Layout.fillWidth: true; height: 1; color: globalBorder1 }

                            // Speed Test Results Row
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 24

                                ColumnLayout {
                                    spacing: 3
                                    Text {
                                        text: "DOWN"
                                        font { pixelSize: 9; family: "Inter" }
                                        font.weight: Font.DemiBold
                                        font.letterSpacing: 1.2
                                        color: pageRoot.textLow
                                    }
                                    Text {
                                        text: NetworkManager.isSpeedTestRunning ? NetworkManager.downloadSpeed : "---"
                                        font { pixelSize: 13; family: "Inter" }
                                        font.weight: Font.Bold
                                        color: NetworkManager.isSpeedTestRunning ? pageRoot.green : pageRoot.textMid
                                    }
                                }

                                ColumnLayout {
                                    spacing: 3
                                    Text {
                                        text: "UP"
                                        font { pixelSize: 9; family: "Inter" }
                                        font.weight: Font.DemiBold
                                        font.letterSpacing: 1.2
                                        color: pageRoot.textLow
                                    }
                                    Text {
                                        text: NetworkManager.isSpeedTestRunning ? NetworkManager.uploadSpeed : "---"
                                        font { pixelSize: 13; family: "Inter" }
                                        font.weight: Font.Bold
                                        color: NetworkManager.isSpeedTestRunning ? pageRoot.accent : pageRoot.textMid
                                    }
                                }

                                ColumnLayout {
                                    spacing: 3
                                    Text {
                                        text: "PING"
                                        font { pixelSize: 9; family: "Inter" }
                                        font.weight: Font.DemiBold
                                        font.letterSpacing: 1.2
                                        color: pageRoot.textLow
                                    }
                                    Text {
                                        text: NetworkManager.isSpeedTestRunning ? (NetworkManager.pingMs >= 0 ? NetworkManager.pingMs + " ms" : "---") : "---"
                                        font { pixelSize: 13; family: "Inter" }
                                        font.weight: Font.Bold
                                        color: NetworkManager.isSpeedTestRunning ? pageRoot.orange : pageRoot.textMid
                                    }
                                }
                            }
                        }
                    }

                    // AVAILABLE NETWORKS Card
                    SettingsCard {
                        Layout.fillWidth: true
                        title: "AVAILABLE NETWORKS"

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Repeater {
                                model: NetworkManager.scannedNetworks
                                delegate: Rectangle {
                                    Layout.fillWidth: true; height: 48; radius: 8
                                    color: netH.containsMouse ? globalBg4 : globalBg3
                                    border.width: 1
                                    border.color: modelData.isConnected ? pageRoot.green + "60" : globalBorder0

                                    RowLayout {
                                        anchors { fill: parent; leftMargin: 14; rightMargin: 14 }
                                        spacing: 12

                                        Text {
                                            text: modelData.ssid
                                            font { pixelSize: 13; family: "Inter" }
                                            font.weight: modelData.isConnected ? Font.Bold : Font.Medium
                                            color: pageRoot.textHigh
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            visible: modelData.isConnected
                                            text: "Connected"
                                            font { pixelSize: 12; family: "Inter" }
                                            font.weight: Font.DemiBold
                                            color: pageRoot.green
                                        }

                                        TitanButton {
                                            visible: !modelData.isConnected
                                            text: "Connect"
                                            primary: false
                                            width: 76
                                            onClicked: openConnectDialog(modelData.ssid, modelData.security)
                                        }
                                    }

                                    MouseArea {
                                        id: netH; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            if (!modelData.isConnected) openConnectDialog(modelData.ssid, modelData.security)
                                        }
                                    }
                                }
                            }

                            // Empty state text if no networks or wifi disabled
                            Text {
                                visible: !NetworkManager.wifiEnabled || NetworkManager.scannedNetworks.length === 0
                                text: !NetworkManager.wifiEnabled ? "Wi-Fi is turned off" : (NetworkManager.isScanning ? "Scanning for networks..." : "No networks found")
                                font { pixelSize: 12; family: "Inter"; italic: true }
                                color: pageRoot.textLow
                                Layout.alignment: Qt.AlignHCenter
                                Layout.topMargin: 8; Layout.bottomMargin: 8
                            }
                        }
                    }
                }

                // Right Column (Connection Details)
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 380
                    spacing: 16
                    Layout.alignment: Qt.AlignTop

                    SettingsCard {
                        Layout.fillWidth: true
                        title: "CONNECTION DETAILS"

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 14

                            // Key-Value rows
                            Repeater {
                                model: [
                                    { label: "IPv4",    val: NetworkManager.ipAddress.length > 0 ? NetworkManager.ipAddress : "—" },
                                    { label: "IPv6",    val: NetworkManager.ipv6Address.length > 0 ? NetworkManager.ipv6Address : "—" },
                                    { label: "Gateway", val: NetworkManager.gatewayAddress.length > 0 ? NetworkManager.gatewayAddress : "—" },
                                    { label: "MAC",     val: NetworkManager.macAddress.length > 0 ? NetworkManager.macAddress : "—" }
                                ]
                                delegate: RowLayout {
                                    Layout.fillWidth: true

                                    Text {
                                        text: modelData.label
                                        font { pixelSize: 13; family: "Inter" }
                                        color: pageRoot.textLow
                                    }

                                    Item { Layout.fillWidth: true }

                                    Text {
                                        text: modelData.val
                                        font { pixelSize: 13; family: "Inter" }
                                        font.weight: Font.Medium
                                        color: pageRoot.textHigh
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            Item { height: 10 }

                            // Disconnect Button
                            Rectangle {
                                Layout.fillWidth: true; height: 40; radius: 8
                                color: "#2B1619"
                                border.width: 1; border.color: "#4A2227"
                                visible: NetworkManager.isConnected || NetworkManager.ethernetConnected

                                Text {
                                    anchors.centerIn: parent
                                    text: "Disconnect"
                                    font { pixelSize: 13; family: "Inter" }
                                    font.weight: Font.Medium
                                    color: pageRoot.red
                                }

                                MouseArea {
                                    anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                    onClicked: NetworkManager.disconnectNetwork()
                                }
                            }
                        }
                    }
                }
            }

            Item { height: 28 }
        }
    }

    // ── Password Modal ───────────────────────────────────────────────
    Item {
        anchors.fill: parent
        visible: showPasswordPrompt
        z: 999

        Rectangle {
            anchors.fill: parent; color: "#B0000000"
            MouseArea { anchors.fill: parent; onClicked: showPasswordPrompt = false }
        }

        Rectangle {
            anchors.centerIn: parent; width: 380; height: 220; radius: 14
            color: globalBg1; border.width: 1; border.color: pageRoot.accent

            ColumnLayout {
                anchors.fill: parent; anchors.margins: 20; spacing: 12

                Text {
                    text: "Wi-Fi Authentication"
                    font { pixelSize: 15; family: "Inter"; weight: Font.Bold }
                    color: pageRoot.textHigh
                }

                Text {
                    text: "Enter password for \"" + targetSsid + "\""
                    font { pixelSize: 12; family: "Inter" }
                    color: pageRoot.textMid
                }

                Rectangle {
                    Layout.fillWidth: true; height: 40; radius: 8
                    color: globalBg3; border.width: 1; border.color: pwdInput.activeFocus ? pageRoot.accent : globalBorder0

                    TextInput {
                        id: pwdInput
                        anchors.fill: parent; anchors.margins: 10
                        text: passwordInput
                        echoMode: hidePasswordChars ? TextInput.Password : TextInput.Normal
                        font { pixelSize: 13; family: "Inter" }
                        color: pageRoot.textHigh
                        onTextChanged: passwordInput = text
                    }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 10
                    TitanButton { text: "Cancel"; primary: false; Layout.fillWidth: true; onClicked: showPasswordPrompt = false }
                    TitanButton { text: "Connect"; primary: true; Layout.fillWidth: true; onClicked: { showPasswordPrompt = false; NetworkManager.connectToNetwork(targetSsid, passwordInput) } }
                }
            }
        }
    }

    // ── Toast Banner ─────────────────────────────────────────────────
    Rectangle {
        anchors { bottom: parent.bottom; bottomMargin: 24; horizontalCenter: parent.horizontalCenter }
        width: Math.min(pageRoot.width - 48, toastTextItem.implicitWidth + 40)
        height: 40; radius: 20
        color: toastIsError ? "#2A1418" : "#142A1E"
        border.width: 1; border.color: toastIsError ? pageRoot.red : pageRoot.green
        visible: toastText.length > 0; z: 1000

        RowLayout {
            anchors.centerIn: parent; spacing: 8
            Rectangle { width: 6; height: 6; radius: 3; color: toastIsError ? pageRoot.red : pageRoot.green }
            Text {
                id: toastTextItem
                text: toastText
                font { pixelSize: 12; family: "Inter"; weight: Font.Medium }
                color: toastIsError ? pageRoot.red : pageRoot.green
            }
        }
    }
}
