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
    property color red:      "#E05C6A"
    property color green:    "#4CAF82"

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
                text: "Audio"
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
            color: globalBg3
            border.width: 1; border.color: globalBorder1

            RowLayout {
                anchors.fill: parent
                spacing: 0

                // OUTPUT
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: 16
                    spacing: 4

                    Text {
                        text: "OUTPUT"
                        font { pixelSize: 9; family: "Inter" }
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.2
                        color: root.textLow
                    }

                    Text {
                        text: AudioBackend.masterMuted ? "Muted" : (AudioBackend.masterVolume + "%")
                        font { pixelSize: 16; family: "Inter" }
                        font.weight: Font.Bold
                        color: AudioBackend.masterMuted ? root.red : root.textHigh
                    }
                }

                Rectangle { width: 1; height: 44; color: globalBorder1 }

                // MICROPHONE
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: 16
                    spacing: 4

                    Text {
                        text: "MICROPHONE"
                        font { pixelSize: 9; family: "Inter" }
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.2
                        color: root.textLow
                    }

                    Text {
                        text: AudioBackend.micMuted ? "Muted" : (AudioBackend.micVolume + "%")
                        font { pixelSize: 16; family: "Inter" }
                        font.weight: Font.Bold
                        color: AudioBackend.micMuted ? root.red : root.textHigh
                    }
                }

                Rectangle { width: 1; height: 44; color: globalBorder1 }

                // EQ PROFILE
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: 16
                    spacing: 4

                    Text {
                        text: "EQ PROFILE"
                        font { pixelSize: 9; family: "Inter" }
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.2
                        color: root.textLow
                    }

                    Text {
                        text: AudioBackend.activeEqProfile
                        font { pixelSize: 15; family: "Inter" }
                        font.weight: Font.Bold
                        color: root.textHigh
                    }
                }

                Rectangle { width: 1; height: 44; color: globalBorder1 }

                // STEREO WIDTH
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: 16; Layout.rightMargin: 16
                    spacing: 4

                    Text {
                        text: "STEREO WIDTH"
                        font { pixelSize: 9; family: "Inter" }
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.2
                        color: root.textLow
                    }

                    Text {
                        text: AudioBackend.spatialWidth + "%"
                        font { pixelSize: 16; family: "Inter" }
                        font.weight: Font.Bold
                        color: root.accent
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

            // Left Column (Output, Microphone, Equalizer)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 600
                spacing: 16
                Layout.alignment: Qt.AlignTop

                // OUTPUT Card
                SettingsCard {
                    Layout.fillWidth: true
                    title: "OUTPUT"

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            TitanSlider {
                                Layout.fillWidth: true
                                from: 0; to: 100; stepSize: 1
                                value: AudioBackend.masterVolume
                                onValueChanged: AudioBackend.masterVolume = value
                                enabled: !AudioBackend.masterMuted
                                opacity: AudioBackend.masterMuted ? 0.35 : 1.0
                                fillColor: AudioBackend.masterMuted ? "#3A3A3A" : root.accent
                            }

                            Text {
                                text: AudioBackend.masterMuted ? "Muted" : (AudioBackend.masterVolume + "%")
                                font { pixelSize: 13; family: "Inter" }
                                font.weight: Font.DemiBold
                                color: AudioBackend.masterMuted ? root.red : root.accent
                                Layout.preferredWidth: 48
                                horizontalAlignment: Text.AlignRight
                            }
                        }

                        Text {
                            text: AudioBackend.activeOutput
                            font { pixelSize: 11; family: "Inter" }
                            color: root.textLow
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }

                // MICROPHONE Card
                SettingsCard {
                    Layout.fillWidth: true
                    title: "MICROPHONE"

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        TitanSlider {
                            Layout.fillWidth: true
                            from: 0; to: 100; stepSize: 1
                            value: AudioBackend.micVolume
                            onValueChanged: AudioBackend.micVolume = value
                            enabled: !AudioBackend.micMuted
                            opacity: AudioBackend.micMuted ? 0.35 : 1.0
                            fillColor: AudioBackend.micMuted ? "#3A3A3A" : root.accent
                        }

                        Text {
                            text: AudioBackend.micMuted ? "Muted" : (AudioBackend.micVolume + "%")
                            font { pixelSize: 13; family: "Inter" }
                            font.weight: Font.DemiBold
                            color: AudioBackend.micMuted ? root.red : root.accent
                            Layout.preferredWidth: 48
                            horizontalAlignment: Text.AlignRight
                        }
                    }
                }

                // EQUALIZER PROFILE Card
                SettingsCard {
                    Layout.fillWidth: true
                    title: "EQUALIZER PROFILE"

                    Flow {
                        Layout.fillWidth: true
                        spacing: 10

                        Repeater {
                            model: ["Flat", "Bass boost", "Vocal", "Electronic", "Acoustic", "Custom"]
                            delegate: Rectangle {
                                height: 34
                                width: profileLabel.implicitWidth + 28
                                radius: 8
                                property bool sel: AudioBackend.activeEqProfile === modelData
                                color: sel ? root.accent : globalBg4
                                border.width: 1
                                border.color: sel ? root.accent : globalBorder0
                                Behavior on color { ColorAnimation { duration: 120 } }

                                Text {
                                    id: profileLabel
                                    anchors.centerIn: parent
                                    text: modelData
                                    font { pixelSize: 12; family: "Inter" }
                                    font.weight: sel ? Font.Medium : Font.Normal
                                    color: sel ? "#FFFFFF" : root.textMid
                                }

                                MouseArea {
                                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                    onClicked: AudioBackend.activeEqProfile = modelData
                                }
                            }
                        }
                    }
                }
            }

            // Right Column (Spatial Audio)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 380
                spacing: 16
                Layout.alignment: Qt.AlignTop

                SettingsCard {
                    Layout.fillWidth: true
                    title: "SPATIAL AUDIO"

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 14

                        RowLayout {
                            Layout.fillWidth: true
                            ColumnLayout {
                                spacing: 3
                                Text {
                                    text: "Virtual stereo widening"
                                    font { pixelSize: 13; family: "Inter" }
                                    font.weight: Font.Medium
                                    color: root.textHigh
                                }
                                Text {
                                    text: "Haas-effect delay. Best with headphones."
                                    font { pixelSize: 12; family: "Inter" }
                                    color: root.textLow
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }
                            }
                            TitanSwitch {
                                onColor: root.accent
                                checked: AudioBackend.spatialAudio
                                onCheckedChanged: AudioBackend.spatialAudio = checked
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: "Width"
                                    font { pixelSize: 12; family: "Inter" }
                                    font.weight: Font.Medium
                                    color: root.textMid
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    text: AudioBackend.spatialWidth + "%"
                                    font { pixelSize: 13; family: "Inter" }
                                    font.weight: Font.DemiBold
                                    color: root.accent
                                }
                            }

                            TitanSlider {
                                Layout.fillWidth: true
                                from: 0; to: 100; stepSize: 1
                                value: AudioBackend.spatialWidth
                                onMoved: AudioBackend.spatialWidth = value
                                fillColor: root.accent
                            }
                        }

                        // Spatial Presets Stack
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Repeater {
                                model: [
                                    { label: "Subtle",  val: 30  },
                                    { label: "Natural", val: 60  },
                                    { label: "Wide",    val: 80  },
                                    { label: "Maximum", val: 100 }
                                ]
                                delegate: Rectangle {
                                    Layout.fillWidth: true
                                    height: 38; radius: 8
                                    property bool sel: AudioBackend.spatialWidth === modelData.val
                                    color: sel ? root.accent : globalBg4
                                    border.width: 1
                                    border.color: sel ? root.accent : globalBorder0
                                    Behavior on color { ColorAnimation { duration: 120 } }

                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.label
                                        font { pixelSize: 13; family: "Inter" }
                                        font.weight: sel ? Font.Medium : Font.Normal
                                        color: sel ? "#FFFFFF" : root.textMid
                                    }

                                    MouseArea {
                                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: AudioBackend.spatialWidth = modelData.val
                                    }
                                }
                            }
                        }

                        Item { height: 4 }

                        // Open Mixer Action Button
                        Rectangle {
                            Layout.fillWidth: true; height: 40; radius: 8
                            color: globalBg4; border.width: 1; border.color: globalBorder0

                            Text {
                                anchors.centerIn: parent
                                text: "Open mixer"
                                font { pixelSize: 13; family: "Inter" }
                                font.weight: Font.Medium
                                color: root.textHigh
                            }

                            MouseArea {
                                anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                onClicked: AudioBackend.openMixer()
                            }
                        }
                    }
                }
            }
        }

        Item { height: 16 }

        // ── Audio Visualizer Section ─────────────────────────────
        SettingsCard {
            Layout.fillWidth: true
            Layout.leftMargin: 24; Layout.rightMargin: 24
            title: "AUDIO VISUALIZER"

            Item {
                Layout.fillWidth: true
                height: 60

                Row {
                    anchors { centerIn: parent }
                    spacing: 6

                    Repeater {
                        model: 18
                        Item {
                            width: 6; height: 50
                            anchors.bottom: parent ? parent.bottom : undefined

                            property real targetH: {
                                if (AudioBackend.masterMuted) return 6;
                                var levels = AudioBackend.eqLevels;
                                if (levels && levels.length > index) {
                                    var val = levels[index];
                                    return 6 + (val / 100.0) * 44;
                                }
                                return 6 + Math.sin((index + Date.now() / 150) * 0.5) * 15 + 15;
                            }
                            
                            Behavior on targetH { NumberAnimation { duration: 60; easing.type: Easing.OutQuad } }

                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: parent.targetH
                                radius: 3
                                color: root.accent
                                opacity: 0.6 + (index / 18) * 0.4
                            }
                        }
                    }
                }
            }
        }

        Item { height: 28 }
    }
}
