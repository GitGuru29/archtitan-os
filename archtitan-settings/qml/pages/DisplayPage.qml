import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import ArchTitanSettings

ScrollView {
    id: root
    contentWidth: -1
    ScrollBar.vertical.policy: ScrollBar.AsNeeded

    property color textHigh: globalTextHigh
    property color textMid:  globalTextMid
    property color textLow:  globalTextLow
    property color accent:   SettingsBackend.accentColor

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
                text: "Display"
                font { pixelSize: 22; family: "Inter" }
                font.weight: Font.Bold
                color: root.textHigh
            }
        }

        Item { height: 24 }

        // ── Brightness Card ──────────────────────────────────────
        SettingsCard {
            Layout.fillWidth: true
            Layout.leftMargin: 24; Layout.rightMargin: 24

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "BRIGHTNESS"
                        font { pixelSize: 10; family: "Inter" }
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.2
                        color: root.textLow
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: Math.round(DisplayManager.brightness) + "%"
                        font { pixelSize: 13; family: "Inter" }
                        font.weight: Font.DemiBold
                        color: root.accent
                    }
                }

                TitanSlider {
                    Layout.fillWidth: true
                    from: 0; to: 100; stepSize: 1
                    value: DisplayManager.brightness
                    onMoved: DisplayManager.brightness = value
                    fillColor: root.accent
                }
            }
        }

        Item { height: 16 }

        // ── Night Light Card ─────────────────────────────────────
        SettingsCard {
            Layout.fillWidth: true
            Layout.leftMargin: 24; Layout.rightMargin: 24

            RowLayout {
                Layout.fillWidth: true
                ColumnLayout {
                    spacing: 3
                    Text {
                        text: "Night light"
                        font { pixelSize: 13; family: "Inter" }
                        font.weight: Font.Medium
                        color: root.textHigh
                    }
                    Text {
                        text: "Reduces blue light via wlsunset"
                        font { pixelSize: 12; family: "Inter" }
                        color: root.textLow
                    }
                }
                Item { Layout.fillWidth: true }
                TitanSwitch {
                    onColor: root.accent
                    checked: DisplayManager.nightLightEnabled
                    onCheckedChanged: DisplayManager.nightLightEnabled = checked
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
                            text: "Color temperature"
                            font { pixelSize: 13; family: "Inter" }
                            font.weight: Font.Medium
                            color: root.textHigh
                            opacity: DisplayManager.nightLightEnabled ? 1.0 : 0.4
                        }
                        Text {
                            text: "Active once night light is on"
                            font { pixelSize: 12; family: "Inter" }
                            color: root.textLow
                            opacity: DisplayManager.nightLightEnabled ? 1.0 : 0.4
                        }
                    }
                    Item { Layout.fillWidth: true }

                    RowLayout {
                        spacing: 8
                        opacity: DisplayManager.nightLightEnabled ? 1.0 : 0.4

                        Rectangle {
                            width: 14; height: 14; radius: 7
                            color: Qt.hsla(0.08, 0.7, 0.5, 1.0)
                        }

                        Text {
                            text: DisplayManager.nightLightTemp + "K"
                            font { pixelSize: 13; family: "Inter" }
                            font.weight: Font.DemiBold
                            color: root.textHigh
                        }
                    }
                }

                TitanSlider {
                    Layout.fillWidth: true
                    from: 1000; to: 6500; stepSize: 100
                    value: DisplayManager.nightLightTemp
                    onMoved: DisplayManager.nightLightTemp = value
                    enabled: DisplayManager.nightLightEnabled
                    opacity: DisplayManager.nightLightEnabled ? 1.0 : 0.4
                    fillColor: Qt.hsla(0.08, 0.7, 0.5, 1.0)
                }
            }
        }

        Item { height: 16 }

        // ── Display Info Card ────────────────────────────────────
        SettingsCard {
            Layout.fillWidth: true
            Layout.leftMargin: 24; Layout.rightMargin: 24

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 14

                Text {
                    text: "DISPLAY INFO"
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
                            { label: "RESOLUTION",   value: DisplayManager.resolution                     },
                            { label: "REFRESH RATE", value: DisplayManager.refreshRate.toFixed(1) + " Hz" }
                        ]
                        delegate: Rectangle {
                            Layout.fillWidth: true; height: 72; radius: 10
                            color: globalBg3; border.width: 1; border.color: globalBorder1

                            ColumnLayout {
                                anchors { fill: parent; margins: 14 }
                                spacing: 4

                                Text {
                                    text: modelData.label
                                    font { pixelSize: 9; family: "Inter" }
                                    font.weight: Font.DemiBold
                                    font.letterSpacing: 1.2
                                    color: root.textLow
                                }

                                Text {
                                    text: modelData.value
                                    font { pixelSize: 20; family: "Inter" }
                                    font.weight: Font.Bold
                                    color: root.textHigh
                                }
                            }
                        }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: globalBorder1 }

                RowLayout {
                    Layout.fillWidth: true
                    ColumnLayout {
                        spacing: 3
                        Text {
                            text: "Display scale"
                            font { pixelSize: 13; family: "Inter" }
                            font.weight: Font.Medium
                            color: root.textHigh
                        }
                        Text {
                            text: "Hyprland monitor scale factor"
                            font { pixelSize: 12; family: "Inter" }
                            color: root.textLow
                        }
                    }
                    Item { Layout.fillWidth: true }

                    RowLayout {
                        spacing: 12
                        TitanSlider {
                            width: 160; from: 0.5; to: 3.0; stepSize: 0.25
                            value: DisplayManager.scaleFactor
                            onMoved: DisplayManager.scaleFactor = value
                            fillColor: root.accent
                        }
                        Text {
                            text: DisplayManager.scaleFactor.toFixed(2) + "×"
                            font { pixelSize: 13; family: "Inter" }
                            font.weight: Font.DemiBold
                            color: root.accent
                        }
                    }
                }
            }
        }

        Item { height: 28 }
    }
}
