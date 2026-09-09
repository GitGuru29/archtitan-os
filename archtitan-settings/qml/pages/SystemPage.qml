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
    property color green:    "#4CAF82"
    property color orange:   "#D4853A"
    property color red:      "#E05C6A"

    ColumnLayout {
        width: root.availableWidth; spacing: 0

        Item { height: 28 }

        // ── Resource meters ──────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true; Layout.leftMargin: 24; Layout.rightMargin: 24
            spacing: 12

            Repeater {
                model: [
                    { label: "CPU",  icon: "cpu",  suffix: "%",   colorFn: 1 },
                    { label: "RAM",  icon: "ram",  suffix: " MB", colorFn: 2 },
                    { label: "DISK", icon: "disk", suffix: " GB", colorFn: 3 }
                ]
                delegate: Rectangle {
                    Layout.fillWidth: true; height: 130; radius: 10
                    color: globalBg3; border.width: 1; border.color: globalBorder1

                    property real val: modelData.label === "CPU" ? SystemInfo.cpuUsage :
                                       modelData.label === "RAM" ? SystemInfo.usedRam :
                                       modelData.label === "DISK" ? SystemInfo.diskUsedGb : 0
                                       
                    property real maxVal: modelData.label === "CPU" ? 100 :
                                          modelData.label === "RAM" ? SystemInfo.totalRam :
                                          modelData.label === "DISK" ? SystemInfo.diskTotalGb : 1
                    property real pct: maxVal > 0 ? val / maxVal : 0
                    property color barColor: pct > 0.8 ? root.red : pct > 0.5 ? root.orange : root.accent

                    ColumnLayout {
                        anchors { fill: parent; margins: 14 }
                        spacing: 6

                        RowLayout {
                            spacing: 8
                            Layout.fillWidth: true

                            Rectangle {
                                width: 26; height: 26; radius: 6
                                color: Qt.hsla(barColor.hslHue, barColor.hslSaturation, barColor.hslLightness, 0.15)
                                border.width: 1; border.color: Qt.hsla(barColor.hslHue, barColor.hslSaturation, barColor.hslLightness, 0.3)

                                Image {
                                    anchors.centerIn: parent
                                    width: 14; height: 14
                                    source: "qrc:/ArchTitanSettings/assets/icons/" + modelData.icon + ".svg"
                                    smooth: true
                                    opacity: 0.9
                                }
                            }

                            Text {
                                text: modelData.label
                                font { pixelSize: 11; family: "Inter" }
                                font.weight: Font.Bold
                                font.letterSpacing: 1.2
                                color: root.textLow
                                Layout.fillWidth: true
                            }
                        }

                        Text {
                            text: {
                                if (modelData.label === "CPU" || modelData.label === "DISK")
                                    return val.toFixed(1) + modelData.suffix
                                return val + modelData.suffix
                            }
                            font { pixelSize: 24; family: "Inter" }
                            font.weight: Font.Bold
                            color: barColor
                        }

                        Rectangle {
                            Layout.fillWidth: true; height: 5; radius: 3; color: globalBorder0
                            Rectangle {
                                width: parent.parent.parent.pct * parent.width
                                height: parent.height; radius: parent.radius
                                color: parent.parent.parent.barColor
                                Behavior on width { NumberAnimation { duration: 600 } }
                            }
                        }

                        Text {
                            text: "of " + (modelData.label === "DISK" ? maxVal.toFixed(1) : maxVal) + modelData.suffix
                            font { pixelSize: 10; family: "Inter" }
                            color: root.textLow
                            visible: maxVal > 0 && modelData.label !== "CPU"
                        }
                    }
                }
            }
        }

        Item { height: 16 }

        // ── System info ──────────────────────────────────────────
        SettingsCard {
            Layout.fillWidth: true; Layout.leftMargin: 24; Layout.rightMargin: 24
            title: "System Information"

            GridLayout {
                Layout.fillWidth: true
                columns: 2; rowSpacing: 18; columnSpacing: 32

                Repeater {
                    model: [
                        { label: "Hostname", icon: "hostname" },
                        { label: "Kernel",   icon: "kernel"   },
                        { label: "CPU",      icon: "cpu"      },
                        { label: "GPU",      icon: "gpu"      },
                        { label: "Uptime",   icon: "uptime"   },
                        { label: "OS",       icon: "os"       }
                    ]
                    delegate: RowLayout {
                        spacing: 12
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop

                        Rectangle {
                            width: 36; height: 36; radius: 8
                            color: globalBg4; border.width: 1; border.color: globalBorder1
                            Layout.alignment: Qt.AlignTop

                            Image {
                                anchors.centerIn: parent
                                width: 18; height: 18
                                source: "qrc:/ArchTitanSettings/assets/icons/" + modelData.icon + ".svg"
                                smooth: true
                                opacity: 0.85
                            }
                        }

                        ColumnLayout {
                            spacing: 3
                            Layout.fillWidth: true

                            Text {
                                text: modelData.label.toUpperCase()
                                font { pixelSize: 9; family: "Inter" }
                                font.weight: Font.Bold
                                font.letterSpacing: 1.2
                                color: root.textLow
                            }

                            Text {
                                text: {
                                    if (modelData.label === "Hostname") return SystemInfo.hostname;
                                    if (modelData.label === "Kernel") return SystemInfo.kernelVersion;
                                    if (modelData.label === "CPU") return SystemInfo.cpuModel;
                                    if (modelData.label === "GPU") return SystemInfo.gpuModel;
                                    if (modelData.label === "Uptime") return SystemInfo.uptime;
                                    if (modelData.label === "OS") return SystemInfo.osVersion;
                                    return "";
                                }
                                font { pixelSize: 13; family: "Inter" }
                                font.weight: Font.Medium
                                color: root.textHigh; elide: Text.ElideRight
                                maximumLineCount: modelData.label === "GPU" ? 3 : 1
                                wrapMode: modelData.label === "GPU" ? Text.WordWrap : Text.NoWrap
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
        }

        Item { height: 12 }

        // ── Titan services ───────────────────────────────────────
        SettingsCard {
            Layout.fillWidth: true; Layout.leftMargin: 24; Layout.rightMargin: 24
            title: "Titan Services"

            Repeater {
                model: [
                    "titan-hwm", "titan-sandboxd", "titanshare-daemon", "NetworkManager", "pipewire"
                ]
                delegate: ColumnLayout {
                    Layout.fillWidth: true; spacing: 0
                    RowLayout {
                        Layout.fillWidth: true; height: 46; spacing: 12
                        Rectangle { width: 7; height: 7; radius: 4; color: root.green; Layout.alignment: Qt.AlignVCenter }
                        Text {
                            text: modelData
                            font { pixelSize: 13; family: "Inter" }
                            font.weight: Font.Medium
                            color: root.textHigh; Layout.fillWidth: true
                        }
                        StatusBadge { text: "Running"; statusColor: root.green }
                    }
                    Rectangle { Layout.fillWidth: true; height: 1; color: globalBorder1; visible: index < 4 }
                }
            }
        }

        Item { height: 28 }
    }
}
