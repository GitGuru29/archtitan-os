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

    ColumnLayout {
        width: root.availableWidth
        spacing: 0

        Item { height: 28 }

        // ── Hero Section ──────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 12

            // Logo Circle
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 96; height: 96
                radius: 48
                color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.12)
                border.width: 1.5
                border.color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.4)

                Image {
                    anchors.centerIn: parent
                    width: 56; height: 56
                    source: "qrc:/ArchTitanSettings/assets/icons/LOGO.png"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }
            }

            Item { height: 4 }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "ArchTitan Settings"
                font { pixelSize: 24; family: "Inter" }
                font.weight: Font.Bold
                color: root.textHigh
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "System Control Center for ArchTitan OS"
                font { pixelSize: 14; family: "Inter" }
                color: root.textSub
            }
        }

        Item { height: 28 }

        // ── Badges Row ──────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 24; Layout.rightMargin: 24
            spacing: 16

            Repeater {
                model: [
                    { label: "VERSION",  value: "v" + SettingsBackend.version(), color: root.accent   },
                    { label: "PLATFORM", value: "Wayland",                        color: root.accent   },
                    { label: "RUNTIME",  value: "Qt6 / QML",                      color: root.accent   },
                    { label: "LICENSE",  value: "Open Source",                    color: root.accent   }
                ]
                delegate: Rectangle {
                    Layout.fillWidth: true
                    height: 76
                    radius: 10
                    color: isDarkTheme ? "#1A1A1E" : globalBg3
                    border.width: 1
                    border.color: globalBorder1

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 4

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: modelData.value
                            font { pixelSize: 16; family: "Inter" }
                            font.weight: Font.Bold
                            color: modelData.color
                        }
                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: modelData.label
                            font { pixelSize: 9; family: "Inter" }
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.2
                            color: root.textSub
                        }
                    }
                }
            }
        }

        Item { height: 20 }

        // ── System Information Card ──────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 24; Layout.rightMargin: 24
            radius: 10
            color: isDarkTheme ? "#1A1A1E" : globalBg3
            border.width: 1; border.color: globalBorder1
            implicitHeight: sysInfoCol.implicitHeight + 36

            ColumnLayout {
                id: sysInfoCol
                anchors {
                    left: parent.left; right: parent.right; top: parent.top
                    margins: 18
                }
                spacing: 16

                Text {
                    text: "SYSTEM INFORMATION"
                    font { pixelSize: 10; family: "Inter" }
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.2
                    color: root.textSub
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 40

                    // Left Column
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 14

                        ColumnLayout {
                            spacing: 2
                            Text {
                                text: "OS"
                                font { pixelSize: 9; family: "Inter" }
                                font.weight: Font.DemiBold
                                font.letterSpacing: 1.2
                                color: root.textSub
                            }
                            Text {
                                text: SystemInfo.osVersion
                                font { pixelSize: 14; family: "Inter" }
                                color: root.textHigh
                            }
                        }

                        ColumnLayout {
                            spacing: 2
                            Text {
                                text: "DESKTOP"
                                font { pixelSize: 9; family: "Inter" }
                                font.weight: Font.DemiBold
                                font.letterSpacing: 1.2
                                color: root.textSub
                            }
                            Text {
                                text: "Hyprland (Wayland)"
                                font { pixelSize: 14; family: "Inter" }
                                color: root.textHigh
                            }
                        }

                        ColumnLayout {
                            spacing: 2
                            Text {
                                text: "SHELL"
                                font { pixelSize: 9; family: "Inter" }
                                font.weight: Font.DemiBold
                                font.letterSpacing: 1.2
                                color: root.textSub
                            }
                            Text {
                                text: "fish"
                                font { pixelSize: 14; family: "Inter" }
                                color: root.textHigh
                            }
                        }
                    }

                    // Right Column
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 14

                        ColumnLayout {
                            spacing: 2
                            Text {
                                text: "KERNEL"
                                font { pixelSize: 9; family: "Inter" }
                                font.weight: Font.DemiBold
                                font.letterSpacing: 1.2
                                color: root.textSub
                            }
                            Text {
                                text: SystemInfo.kernelVersion
                                font { pixelSize: 14; family: "Monospace" }
                                color: root.textHigh
                            }
                        }

                        ColumnLayout {
                            spacing: 2
                            Text {
                                text: "PACKAGE MGR"
                                font { pixelSize: 9; family: "Inter" }
                                font.weight: Font.DemiBold
                                font.letterSpacing: 1.2
                                color: root.textSub
                            }
                            Text {
                                text: "pacman + AUR"
                                font { pixelSize: 14; family: "Inter" }
                                color: root.textHigh
                            }
                        }

                        ColumnLayout {
                            spacing: 2
                            Text {
                                text: "AUDIO"
                                font { pixelSize: 9; family: "Inter" }
                                font.weight: Font.DemiBold
                                font.letterSpacing: 1.2
                                color: root.textSub
                            }
                            Text {
                                text: "PipeWire / WirePlumber"
                                font { pixelSize: 14; family: "Inter" }
                                color: root.textHigh
                            }
                        }
                    }
                }
            }
        }

        Item { height: 16 }

        // ── Tech Stack Card ───────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 24; Layout.rightMargin: 24
            radius: 10
            color: isDarkTheme ? "#1A1A1E" : globalBg3
            border.width: 1; border.color: globalBorder1
            implicitHeight: techStackCol.implicitHeight + 36

            ColumnLayout {
                id: techStackCol
                anchors {
                    left: parent.left; right: parent.right; top: parent.top
                    margins: 18
                }
                spacing: 14

                Text {
                    text: "TECH STACK"
                    font { pixelSize: 10; family: "Inter" }
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.2
                    color: root.textSub
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8

                    Repeater {
                        model: [
                            "Qt6 / QML", "C++17", "PipeWire", "WirePlumber",
                            "Hyprland", "Wayland", "systemd", "waybar",
                            "rofi", "kitty", "pacman", "AUR"
                        ]
                        delegate: Rectangle {
                            height: 30; radius: 15
                            width: chipLabel.implicitWidth + 24
                            color: isDarkTheme ? "#24242A" : "#E8E8EE"
                            border.width: 1
                            border.color: globalBorder0

                            Text {
                                id: chipLabel
                                anchors.centerIn: parent
                                text: modelData
                                font { pixelSize: 12; family: "Inter" }
                                color: root.textHigh
                            }
                        }
                    }
                }
            }
        }

        Item { height: 16 }

        // ── Project Links ─────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 24; Layout.rightMargin: 24
            spacing: 16

            Repeater {
                model: [
                    { label: "GitHub",       url: "https://github.com/GitGuru29/archtitan-os"       },
                    { label: "Report issue",  url: "https://github.com/GitGuru29/archtitan-os/issues" },
                    { label: "Wiki",          url: "https://github.com/GitGuru29/archtitan-os/wiki"   }
                ]
                delegate: Rectangle {
                    Layout.fillWidth: true
                    height: 48; radius: 8
                    color: linkArea.containsMouse ? (isDarkTheme ? "#282830" : "#E2E2E8") : (isDarkTheme ? "#1A1A1E" : globalBg3)
                    border.width: 1
                    border.color: globalBorder1
                    Behavior on color { ColorAnimation { duration: 120 } }

                    Text {
                        anchors.centerIn: parent
                        text: modelData.label
                        font { pixelSize: 13; family: "Inter" }
                        font.weight: Font.Medium
                        color: root.textHigh
                    }

                    MouseArea {
                        id: linkArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Qt.openUrlExternally(modelData.url)
                    }
                }
            }
        }

        Item { height: 28 }

        // ── Footer ────────────────────────────────────────────────
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "© 2026 ArchTitan Project"
            font { pixelSize: 12; family: "Inter" }
            color: root.textSub
        }

        Item { height: 28 }
    }
}
