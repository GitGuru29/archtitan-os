import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import ArchTitanSettings

ScrollView {
    id: root
    contentWidth: -1
    ScrollBar.vertical.policy: ScrollBar.AsNeeded

    property color textHigh:  globalTextHigh
    property color textMid:   globalTextMid
    property color textLow:   globalTextLow
    property color accent:    SettingsBackend.accentColor
    property color green:     "#4CAF82"
    property color orange:    "#D4853A"

    readonly property var accentColors: [
        "#4C8BF5", "#00D2D3", "#9575CD", "#66BB6A",
        "#FFA726", "#FC5C65", "#26A69A", "#FED33C"
    ]

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
                text: "Appearance"
                font { pixelSize: 22; family: "Inter" }
                font.weight: Font.Bold
                color: root.textHigh
            }

            Text {
                text: "Customize how ArchTitan OS looks and feels"
                font { pixelSize: 13; family: "Inter" }
                color: root.textLow
            }
        }

        Item { height: 24 }

        // ── Theme Section ────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 24; Layout.rightMargin: 24
            spacing: 10

            Text {
                text: "THEME"
                font { pixelSize: 10; family: "Inter" }
                font.weight: Font.DemiBold
                font.letterSpacing: 1.2
                color: root.textLow
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Repeater {
                    model: [
                        { name: "Dark",     bg: "#1C1C1C" },
                        { name: "Darker",   bg: "#0A0A0A" },
                        { name: "Midnight", bg: "#0D111A" },
                        { name: "Dim",      bg: "#252830" }
                    ]
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        height: 96; radius: 10
                        color: globalBg3
                        border.width: SettingsBackend.colorTheme === modelData.name ? 2 : 1
                        border.color: SettingsBackend.colorTheme === modelData.name
                                      ? root.accent : globalBorder0
                        Behavior on border.color { ColorAnimation { duration: 150 } }

                        ColumnLayout {
                            anchors { fill: parent; margins: 12 }
                            spacing: 8

                            Rectangle {
                                Layout.fillWidth: true
                                height: 42; radius: 6
                                color: modelData.bg
                                border.width: 1; border.color: globalBorder0

                                // Titlebar dots inside preview
                                Row {
                                    anchors { left: parent.left; leftMargin: 8; top: parent.top; topMargin: 6 }
                                    spacing: 4
                                    Repeater {
                                        model: ["#ED6A5E", "#F5BF4F", "#61C554"]
                                        Rectangle { width: 4; height: 4; radius: 2; color: modelData }
                                    }
                                }
                            }

                            Text {
                                text: modelData.name
                                Layout.alignment: Qt.AlignHCenter
                                font { pixelSize: 12; family: "Inter" }
                                font.weight: SettingsBackend.colorTheme === modelData.name ? Font.Medium : Font.Normal
                                color: SettingsBackend.colorTheme === modelData.name ? root.textHigh : root.textMid
                            }
                        }

                        MouseArea {
                            anchors.fill: parent; hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: SettingsBackend.colorTheme = modelData.name
                        }
                    }
                }
            }
        }

        Item { height: 24 }

        // ── Accent Color Section ──────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 24; Layout.rightMargin: 24
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "ACCENT COLOR"
                    font { pixelSize: 10; family: "Inter" }
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.2
                    color: root.textLow
                }
                Item { Layout.fillWidth: true }

                Rectangle {
                    height: 24; width: hexText.implicitWidth + 16; radius: 6
                    color: Qt.hsla(accent.hslHue, accent.hslSaturation, accent.hslLightness, 0.15)
                    border.width: 1; border.color: Qt.hsla(accent.hslHue, accent.hslSaturation, accent.hslLightness, 0.3)

                    Text {
                        id: hexText
                        anchors.centerIn: parent
                        text: SettingsBackend.accentColor.toUpperCase()
                        font { pixelSize: 11; family: "Inter" }
                        font.weight: Font.DemiBold
                        color: root.accent
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 14

                Repeater {
                    model: root.accentColors
                    delegate: Item {
                        width: 32; height: 32
                        
                        // Outer selection ring
                        Rectangle {
                            anchors.centerIn: parent
                            width: 32; height: 32; radius: 16
                            color: "transparent"
                            border.width: SettingsBackend.accentColor === modelData ? 2 : 0
                            border.color: root.accent
                            Behavior on border.width { NumberAnimation { duration: 120 } }
                        }

                        Rectangle {
                            anchors.centerIn: parent
                            width: SettingsBackend.accentColor === modelData ? 24 : 28
                            height: width; radius: width / 2
                            color: modelData
                            Behavior on width { NumberAnimation { duration: 120 } }

                            MouseArea {
                                anchors.fill: parent; hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: SettingsBackend.accentColor = modelData
                            }
                        }
                    }
                }
            }
        }

        Item { height: 24 }

        // ── Visual Effects Section ───────────────────────────────
        SettingsCard {
            Layout.fillWidth: true
            Layout.leftMargin: 24; Layout.rightMargin: 24
            title: "VISUAL EFFECTS"

            RowLayout {
                Layout.fillWidth: true
                ColumnLayout {
                    spacing: 3
                    Text {
                        text: "Blur / transparency"
                        font { pixelSize: 13; family: "Inter" }
                        font.weight: Font.Medium
                        color: root.textHigh
                    }
                    Text {
                        text: "Frosted glass effect on panels"
                        font { pixelSize: 12; family: "Inter" }
                        color: root.textLow
                    }
                }
                Item { Layout.fillWidth: true }
                TitanSwitch {
                    onColor: root.accent
                    checked: SettingsBackend.glassmorphism
                    onCheckedChanged: SettingsBackend.glassmorphism = checked
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: globalBorder1; Layout.topMargin: 8; Layout.bottomMargin: 8 }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    ColumnLayout {
                        spacing: 3
                        Text {
                            text: "Panel opacity"
                            font { pixelSize: 13; family: "Inter" }
                            font.weight: Font.Medium
                            color: root.textHigh
                        }
                        Text {
                            text: "Transparency level for sidebars"
                            font { pixelSize: 12; family: "Inter" }
                            color: root.textLow
                        }
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: Math.round(SettingsBackend.panelOpacity * 100) + "%"
                        font { pixelSize: 13; family: "Inter" }
                        font.weight: Font.DemiBold
                        color: root.accent
                    }
                }

                TitanSlider {
                    Layout.fillWidth: true
                    from: 0.4; to: 1.0; stepSize: 0.01
                    fillColor: root.accent
                    value: SettingsBackend.panelOpacity
                    onValueChanged: SettingsBackend.panelOpacity = value
                }
            }
        }

        Item { height: 24 }

        // ── Icon Theme Section ────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 24; Layout.rightMargin: 24
            spacing: 10

            Text {
                text: "ICON THEME"
                font { pixelSize: 10; family: "Inter" }
                font.weight: Font.DemiBold
                font.letterSpacing: 1.2
                color: root.textLow
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Repeater {
                    model: ["Papirus-dark", "Papirus", "Adwaita", "Breeze-dark"]
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        height: 84; radius: 10
                        color: globalBg3
                        border.width: SettingsBackend.iconTheme === modelData ? 2 : 1
                        border.color: SettingsBackend.iconTheme === modelData ? root.accent : globalBorder0
                        Behavior on border.color { ColorAnimation { duration: 150 } }

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: 8

                            // Folder icon preview badge
                            Rectangle {
                                Layout.alignment: Qt.AlignHCenter
                                width: 36; height: 26; radius: 5
                                color: modelData === "Adwaita" ? "#E0A93B" :
                                       modelData === "Papirus-dark" ? "#3B82F6" :
                                       modelData === "Papirus" ? "#60A5FA" : "#38BDF8"

                                Rectangle {
                                    x: 3; y: -3; width: 14; height: 8; radius: 2
                                    color: Qt.lighter(parent.color, 1.2)
                                }
                            }

                            Text {
                                text: modelData
                                Layout.alignment: Qt.AlignHCenter
                                font { pixelSize: 12; family: "Inter" }
                                font.weight: SettingsBackend.iconTheme === modelData ? Font.Medium : Font.Normal
                                color: SettingsBackend.iconTheme === modelData ? root.textHigh : root.textMid
                            }
                        }

                        MouseArea {
                            anchors.fill: parent; hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: SettingsBackend.iconTheme = modelData
                        }
                    }
                }
            }
        }

        Item { height: 24 }

        // ── Typography Section ────────────────────────────────────
        SettingsCard {
            Layout.fillWidth: true
            Layout.leftMargin: 24; Layout.rightMargin: 24
            title: "TYPOGRAPHY"

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: "Font family"
                    font { pixelSize: 13; family: "Inter" }
                    font.weight: Font.Medium
                    color: root.textHigh
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Repeater {
                        model: ["Inter", "JetBrains Mono", "Fira Code", "Roboto", "Noto Sans"]
                        delegate: Rectangle {
                            height: 32
                            Layout.fillWidth: true
                            radius: 6
                            color: SettingsBackend.fontFamily === modelData ? root.accent : globalBg4
                            border.width: 1
                            border.color: SettingsBackend.fontFamily === modelData ? root.accent : globalBorder0

                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                font { pixelSize: 12; family: modelData }
                                font.weight: SettingsBackend.fontFamily === modelData ? Font.Medium : Font.Normal
                                color: SettingsBackend.fontFamily === modelData ? "#FFFFFF" : root.textMid
                            }

                            MouseArea {
                                anchors.fill: parent; hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: SettingsBackend.fontFamily = modelData
                            }
                        }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: globalBorder1; Layout.topMargin: 8; Layout.bottomMargin: 8 }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    ColumnLayout {
                        spacing: 3
                        Text {
                            text: "Font size"
                            font { pixelSize: 13; family: "Inter" }
                            font.weight: Font.Medium
                            color: root.textHigh
                        }
                        Text {
                            text: "System-wide interface font size"
                            font { pixelSize: 12; family: "Inter" }
                            color: root.textLow
                        }
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: SettingsBackend.fontSize + " pt"
                        font { pixelSize: 13; family: "Inter" }
                        font.weight: Font.DemiBold
                        color: root.accent
                    }
                }

                TitanSlider {
                    Layout.fillWidth: true
                    from: 9; to: 18; stepSize: 1
                    fillColor: root.accent
                    value: SettingsBackend.fontSize
                    onValueChanged: SettingsBackend.fontSize = value
                }
            }
        }

        Item { height: 24 }

        // ── Action Buttons ────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 24; Layout.rightMargin: 24
            Item { Layout.fillWidth: true }
            TitanButton { text: "Reset Defaults"; primary: false; width: 140; onClicked: SettingsBackend.resetToDefaults() }
            Item { width: 10 }
            TitanButton { text: "Apply & Save"; primary: true; width: 130; onClicked: SettingsBackend.applyAppearance() }
        }

        Item { height: 28 }
    }
}
