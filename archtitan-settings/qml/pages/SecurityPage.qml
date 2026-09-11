import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import ArchTitanSettings

ScrollView {
    id: root
    contentWidth: -1
    ScrollBar.vertical.policy: ScrollBar.AsNeeded

    property color textHigh: isDarkTheme ? "#FFFFFF" : "#111111"
    property color textMid:  isDarkTheme ? "#CCCCCC" : "#444444"
    property color textSub:  isDarkTheme ? "#A0A0A8" : "#666666"
    property color textLow:  isDarkTheme ? "#808088" : "#888888"
    property color accent:   SettingsBackend.accentColor
    property color green:    "#4CAF82"
    property color red:      "#E05C6A"

    ColumnLayout {
        width: root.availableWidth
        spacing: 0

        Item { height: 20 }

        // ── Page Header ──────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 24; Layout.rightMargin: 24
            spacing: 4

            Text {
                text: "Security"
                font { pixelSize: 22; family: "Inter" }
                font.weight: Font.Bold
                color: root.textHigh
            }
        }

        Item { height: 20 }

        // ── Top Stats Bar ──────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 24; Layout.rightMargin: 24
            height: 76; radius: 10
            color: isDarkTheme ? "#1A1A1E" : globalBg3
            border.width: 1; border.color: globalBorder1

            RowLayout {
                anchors.fill: parent
                spacing: 0

                // AUTOLOCK
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: 16
                    spacing: 4

                    Text {
                        text: "AUTOLOCK"
                        font { pixelSize: 9; family: "Inter" }
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.2
                        color: root.textSub
                    }

                    Text {
                        text: SettingsBackend.autolockEnabled 
                              ? "On · " + Math.floor(SettingsBackend.autolockDelay / 60) + "m"
                              : "Off"
                        font { pixelSize: 20; family: "Inter" }
                        font.weight: Font.Bold
                        color: SettingsBackend.autolockEnabled ? root.green : root.textSub
                    }
                }

                Rectangle { width: 1; height: 44; color: globalBorder1 }

                // FEATURES ACTIVE
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: 16
                    spacing: 4

                    Text {
                        text: "FEATURES ACTIVE"
                        font { pixelSize: 9; family: "Inter" }
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.2
                        color: root.textSub
                    }

                    Row {
                        spacing: 2
                        Text {
                            text: "2"
                            font { pixelSize: 22; family: "Inter" }
                            font.weight: Font.Bold
                            color: root.textHigh
                            anchors.baseline: slashFourText.baseline
                        }
                        Text {
                            id: slashFourText
                            text: "/4"
                            font { pixelSize: 13; family: "Inter" }
                            color: root.textSub
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 3
                        }
                    }
                }

                Rectangle { width: 1; height: 44; color: globalBorder1 }

                // SANDBOX
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: 16
                    spacing: 4

                    Text {
                        text: "SANDBOX"
                        font { pixelSize: 9; family: "Inter" }
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.2
                        color: root.textSub
                    }

                    Text {
                        text: "2 running"
                        font { pixelSize: 20; family: "Inter" }
                        font.weight: Font.Bold
                        color: root.green
                    }
                }

                Rectangle { width: 1; height: 44; color: globalBorder1 }

                // DISK ENCRYPTION
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: 16; Layout.rightMargin: 16
                    spacing: 4

                    Text {
                        text: "DISK ENCRYPTION"
                        font { pixelSize: 9; family: "Inter" }
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.2
                        color: root.textSub
                    }

                    Text {
                        text: "Inactive"
                        font { pixelSize: 20; family: "Inter" }
                        font.weight: Font.Bold
                        color: root.red
                    }
                }
            }
        }

        Item { height: 24 }

        // ── 2-Column Main Section ──────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 24; Layout.rightMargin: 24
            spacing: 24
            Layout.alignment: Qt.AlignTop

            // Left Column (Screen Lock, Appearance, Quick Actions)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 580
                spacing: 24
                Layout.alignment: Qt.AlignTop

                // SCREEN LOCK
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    Text {
                        text: "SCREEN LOCK"
                        font { pixelSize: 10; family: "Inter" }
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.2
                        color: root.textSub
                    }

                    // Screen Autolock
                    RowLayout {
                        Layout.fillWidth: true

                        ColumnLayout {
                            spacing: 4
                            Layout.fillWidth: true

                            Text {
                                text: "Screen autolock"
                                font { pixelSize: 14; family: "Inter" }
                                color: root.textHigh
                            }
                            Text {
                                text: "Lock when idle using swaylock"
                                font { pixelSize: 12; family: "Inter" }
                                color: root.textSub
                            }
                        }
                        Item { width: 8 }
                        TitanSwitch {
                            onColor: root.accent
                            checked: SettingsBackend.autolockEnabled
                            onCheckedChanged: SettingsBackend.autolockEnabled = checked
                        }
                    }

                    // Lock After
                    RowLayout {
                        Layout.fillWidth: true
                        opacity: SettingsBackend.autolockEnabled ? 1.0 : 0.4

                        ColumnLayout {
                            spacing: 4
                            Layout.fillWidth: true

                            Text {
                                text: "Lock after"
                                font { pixelSize: 14; family: "Inter" }
                                color: root.textHigh
                            }
                            Text {
                                text: "Idle time before screen locks"
                                font { pixelSize: 12; family: "Inter" }
                                color: root.textSub
                            }
                        }
                        Item { width: 8 }
                        
                        // Clickable delay selector button
                        Rectangle {
                            height: 28; width: 56; radius: 6
                            color: isDarkTheme ? "#222228" : "#E8E8EE"
                            border.width: 1; border.color: globalBorder0

                            Text {
                                anchors.centerIn: parent
                                text: Math.floor(SettingsBackend.autolockDelay / 60) + "m"
                                font { pixelSize: 13; family: "Inter" }
                                font.weight: Font.Medium
                                color: root.accent
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                enabled: SettingsBackend.autolockEnabled
                                onClicked: {
                                    var current = SettingsBackend.autolockDelay;
                                    if (current >= 1800) SettingsBackend.autolockDelay = 60;
                                    else if (current >= 900) SettingsBackend.autolockDelay = 1800;
                                    else if (current >= 600) SettingsBackend.autolockDelay = 900;
                                    else if (current >= 300) SettingsBackend.autolockDelay = 600;
                                    else if (current >= 120) SettingsBackend.autolockDelay = 300;
                                    else SettingsBackend.autolockDelay = 120;
                                }
                            }
                        }
                    }

                    // Lock on Screen Off
                    RowLayout {
                        Layout.fillWidth: true

                        ColumnLayout {
                            spacing: 4
                            Layout.fillWidth: true

                            Text {
                                text: "Lock on screen off"
                                font { pixelSize: 14; family: "Inter" }
                                color: root.textHigh
                            }
                            Text {
                                text: "Password required immediately"
                                font { pixelSize: 12; family: "Inter" }
                                color: root.textSub
                            }
                        }
                        Item { width: 8 }
                        TitanSwitch {
                            onColor: root.accent
                            checked: SettingsBackend.lockOnScreenOff
                            onCheckedChanged: SettingsBackend.lockOnScreenOff = checked
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: globalBorder1 }

                // LOCKSCREEN APPEARANCE
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    Text {
                        text: "LOCKSCREEN APPEARANCE"
                        font { pixelSize: 10; family: "Inter" }
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.2
                        color: root.textSub
                    }

                    // Blur Background
                    RowLayout {
                        Layout.fillWidth: true

                        ColumnLayout {
                            spacing: 4
                            Layout.fillWidth: true

                            Text {
                                text: "Blur background"
                                font { pixelSize: 14; family: "Inter" }
                                color: root.textHigh
                            }
                            Text {
                                text: "Blur the wallpaper behind the lockscreen"
                                font { pixelSize: 12; family: "Inter" }
                                color: root.textSub
                            }
                        }
                        Item { width: 8 }
                        TitanSwitch {
                            onColor: root.accent
                            checked: SettingsBackend.lockscreenBlur
                            onCheckedChanged: SettingsBackend.lockscreenBlur = checked
                        }
                    }

                    // Ring Color
                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: "Ring color"
                            font { pixelSize: 14; family: "Inter" }
                            color: root.textHigh
                            Layout.fillWidth: true
                        }

                        Row {
                            spacing: 10
                            Repeater {
                                model: [
                                    { colorHex: SettingsBackend.accentColor.replace("#", ""), val: SettingsBackend.accentColor.replace("#", "") },
                                    { colorHex: "4CAF82", val: "4CAF82" },
                                    { colorHex: "E05C6A", val: "E05C6A" },
                                    { colorHex: "FFFFFF", val: "default" }
                                ]
                                delegate: Rectangle {
                                    width: 28; height: 28; radius: 14
                                    color: "#" + modelData.colorHex
                                    border.width: SettingsBackend.lockscreenRingColor === modelData.val ? 2 : 1
                                    border.color: SettingsBackend.lockscreenRingColor === modelData.val ? root.textHigh : globalBorder0

                                    MouseArea {
                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: SettingsBackend.lockscreenRingColor = modelData.val
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: globalBorder1 }

                // QUICK ACTIONS
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Text {
                        text: "QUICK ACTIONS"
                        font { pixelSize: 10; family: "Inter" }
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.2
                        color: root.textSub
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        // Lock screen button
                        Rectangle {
                            Layout.fillWidth: true; height: 48; radius: 8
                            color: lockArea.containsMouse ? (isDarkTheme ? "#2A2A30" : "#E5E5EB") : (isDarkTheme ? "#1E1E22" : globalBg4)
                            border.width: 1; border.color: globalBorder0
                            Behavior on color { ColorAnimation { duration: 120 } }

                            Text {
                                anchors.centerIn: parent
                                text: "Lock screen"
                                font { pixelSize: 13; family: "Inter" }
                                font.weight: Font.Medium
                                color: root.textHigh
                            }

                            MouseArea {
                                id: lockArea
                                anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                onClicked: Qt.openUrlExternally("exec:swaylock")
                            }
                        }

                        // Change password button
                        Rectangle {
                            Layout.fillWidth: true; height: 48; radius: 8
                            color: pwdArea.containsMouse ? (isDarkTheme ? "#2A2A30" : "#E5E5EB") : (isDarkTheme ? "#1E1E22" : globalBg4)
                            border.width: 1; border.color: globalBorder0
                            Behavior on color { ColorAnimation { duration: 120 } }

                            Text {
                                anchors.centerIn: parent
                                text: "Change password"
                                font { pixelSize: 13; family: "Inter" }
                                font.weight: Font.Medium
                                color: root.textHigh
                            }

                            MouseArea {
                                id: pwdArea
                                anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                onClicked: Qt.openUrlExternally("exec:passwd")
                            }
                        }
                    }
                }
            }

            // Right Column (Security Features, Sandbox Containers)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 360
                spacing: 24
                Layout.alignment: Qt.AlignTop

                // SECURITY FEATURES
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Text {
                        text: "SECURITY FEATURES"
                        font { pixelSize: 10; family: "Inter" }
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.2
                        color: root.textSub
                    }

                    Repeater {
                        model: [
                            { label: "Titan sandbox",      enabled: true  },
                            { label: "Secure boot",         enabled: false },
                            { label: "Disk encryption",     enabled: false },
                            { label: "Firewall (nftables)", enabled: true  }
                        ]
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            height: 32

                            Text {
                                text: modelData.label
                                font { pixelSize: 14; family: "Inter" }
                                color: root.textHigh
                                Layout.fillWidth: true
                            }

                            Rectangle {
                                height: 24; width: badgeText.implicitWidth + 20; radius: 6
                                color: modelData.enabled 
                                       ? (isDarkTheme ? "#152E22" : "#E6F4EA") 
                                       : (isDarkTheme ? "#2E181A" : "#FCE8E6")

                                Text {
                                    id: badgeText
                                    anchors.centerIn: parent
                                    text: modelData.enabled ? "Active" : "Inactive"
                                    font { pixelSize: 11; family: "Inter" }
                                    font.weight: Font.Medium
                                    color: modelData.enabled ? root.green : root.red
                                }
                            }
                        }
                    }
                }

                // SANDBOX CONTAINERS
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Text {
                        text: "SANDBOX CONTAINERS"
                        font { pixelSize: 10; family: "Inter" }
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.2
                        color: root.textSub
                    }

                    Repeater {
                        model: ["titan-browser", "titan-media"]
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            height: 28

                            Text {
                                text: modelData
                                font { pixelSize: 13; family: "Monospace" }
                                color: root.textHigh
                                Layout.fillWidth: true
                            }

                            Text {
                                text: "Running"
                                font { pixelSize: 12; family: "Inter" }
                                font.weight: Font.Medium
                                color: root.green
                            }
                        }
                    }

                    Item { height: 4 }

                    // Open sandbox shell button
                    Rectangle {
                        Layout.fillWidth: true; height: 44; radius: 8
                        color: shellArea.containsMouse ? (isDarkTheme ? "#243248" : "#DCE8FE") : (isDarkTheme ? "#1A2436" : "#EDF3FE")
                        border.width: 1; border.color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.4)
                        Behavior on color { ColorAnimation { duration: 120 } }

                        Text {
                            anchors.centerIn: parent
                            text: "Open sandbox shell"
                            font { pixelSize: 13; family: "Inter" }
                            font.weight: Font.Medium
                            color: root.accent
                        }

                        MouseArea {
                            id: shellArea
                            anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: Qt.openUrlExternally("exec:kitty -e machinectl shell titan-browser")
                        }
                    }
                }
            }
        }

        Item { height: 28 }
    }
}
