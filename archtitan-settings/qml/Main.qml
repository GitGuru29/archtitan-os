import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ArchTitanSettings


ApplicationWindow {
    id: root
    width: 1160
    height: 740
    minimumWidth: 920
    minimumHeight: 580
    visible: true
    title: "ArchTitan Settings"
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint

    // ── Design tokens ──────────────────────────────────────────────
    readonly property bool isDarkTheme: WallpaperManager.isDark

    readonly property color globalBg0:          isDarkTheme ? "#0D0D0D" : "#F5F5F5"
    readonly property color globalBg1:          isDarkTheme ? "#B3111111" : "#B3FFFFFF"
    readonly property color globalBg2:          isDarkTheme ? "#99171717" : "#99E5E5E5"
    readonly property color globalBg3:          isDarkTheme ? "#161616" : "#EAEAEA"
    readonly property color globalBg4:          isDarkTheme ? "#242424" : "#DDDDDD"
    readonly property color globalBorder0:      isDarkTheme ? "#2A2A2A" : "#CCCCCC"
    readonly property color globalBorder1:      isDarkTheme ? "#1F1F1F" : "#E0E0E0"
    readonly property color globalTextHigh:     isDarkTheme ? "#EBEBEB" : "#111111"
    readonly property color globalTextMid:      isDarkTheme ? "#8C8C8C" : "#444444"
    readonly property color globalTextLow:      isDarkTheme ? "#707070" : "#777777"

    readonly property color bg0:          globalBg0
    readonly property color bg1:          globalBg1
    readonly property color bg2:          globalBg2
    readonly property color bg3:          isDarkTheme ? "#1C1C1C" : "#E5E5E5"
    readonly property color bg4:          globalBg4
    readonly property color border0:      globalBorder0
    readonly property color border1:      globalBorder1
    property color accent:       SettingsBackend.accentColor
    property color accentDim:    Qt.rgba(accent.r, accent.g, accent.b, 0.25)
    readonly property color textHigh:     globalTextHigh
    readonly property color textMid:      globalTextMid
    readonly property color textLow:      globalTextLow
    readonly property color green:        "#4CAF82"
    readonly property color red:          "#E05C6A"
    readonly property color orange:       "#D4853A"
    readonly property color purple:       "#7C6FCD"

    readonly property int sidebarW: 220
    property int currentPage: 0

    readonly property var pages: [
        { name: "Appearance",  icon: "appearance", label: "Appearance"  },
        { name: "Display",     icon: "display",    label: "Display"      },
        { name: "Network",     icon: "network",    label: "Network"      },
        { name: "Audio",       icon: "audio",      label: "Audio"        },
        { name: "Power",       icon: "power",      label: "Power"        },
        { name: "Security",    icon: "security",   label: "Security"     },
        { name: "System",      icon: "system",     label: "System"       },
        { name: "About",       icon: "about",      label: "About"        }
    ]

    // ── Window drag ────────────────────────────────────────────────
    MouseArea {
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 52
        property point clickPos
        onPressed:  (mouse) => { clickPos = Qt.point(mouse.x, mouse.y) }
        onPositionChanged: (mouse) => {
            root.x += mouse.x - clickPos.x
            root.y += mouse.y - clickPos.y
        }
        z: -1
    }

    // ── Window shell ───────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        radius: 12
        color: root.bg1
        clip: true

        // Outer border
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: 1
            border.color: root.border0
        }

        RowLayout {
            anchors.fill: parent
            spacing: 0

            // ── Sidebar ───────────────────────────────────────────
            Rectangle {
                Layout.preferredWidth: root.sidebarW
                Layout.fillHeight: true
                color: "#0a0e16"
                radius: 12

                // Square off right corners
                Rectangle {
                    anchors { top: parent.top; bottom: parent.bottom; right: parent.right }
                    width: 12
                    color: parent.color
                }

                // Right border
                Rectangle {
                    anchors { top: parent.top; bottom: parent.bottom; right: parent.right }
                    width: 1
                    color: Qt.rgba(0.298, 0.545, 0.96, 0.25)
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // ── Logo / Header ────────────────────────────
                    Item {
                        Layout.fillWidth: true
                        implicitHeight: logoHeaderCol.implicitHeight + 20

                        ColumnLayout {
                            id: logoHeaderCol
                            anchors {
                                top: parent.top
                                topMargin: 20
                                horizontalCenter: parent.horizontalCenter
                            }
                            spacing: 6

                            // Logo icon badge
                            Rectangle {
                                Layout.alignment: Qt.AlignHCenter
                                width: 34; height: 34
                                radius: 8
                                color: Qt.rgba(0.298, 0.545, 0.96, 0.15)
                                border.width: 1
                                border.color: Qt.rgba(0.298, 0.545, 0.96, 0.4)

                                Image {
                                    anchors.centerIn: parent
                                    width: 22; height: 22
                                    source: "qrc:/ArchTitanSettings/assets/icons/LOGO.png"
                                    fillMode: Image.PreserveAspectFit
                                    smooth: true
                                }
                            }

                            // Wordmark
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: "ArchTitan Settings"
                                font { pixelSize: 13; family: "Inter" }
                                font.weight: Font.Bold
                                color: "#f2f4f8"
                            }
                        }
                    }

                    Item { height: 26 }

                    // ── Nav items ─────────────────────────────────
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Repeater {
                            model: root.pages
                            delegate: SidebarItem {
                                Layout.fillWidth: true
                                iconSource: "qrc:/ArchTitanSettings/assets/icons/" + modelData.icon + ".svg"
                                label: modelData.label
                                active: root.currentPage === index
                                onClicked: root.currentPage = index
                            }
                        }
                    }

                    // Pin footer to bottom regardless of nav item count
                    Item { Layout.fillHeight: true }

                    // ── Footer Status Block ───────────────────────
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Qt.rgba(1, 1, 1, 0.06)
                    }

                    Item {
                        Layout.fillWidth: true
                        implicitHeight: 56

                        RowLayout {
                            anchors {
                                left: parent.left; leftMargin: 18
                                right: parent.right; rightMargin: 16
                                top: parent.top; topMargin: 16
                            }
                            spacing: 10

                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: "#46d183"
                                Layout.alignment: Qt.AlignVCenter
                            }

                            ColumnLayout {
                                spacing: 1
                                Layout.fillWidth: true

                                Text {
                                    text: "ArchTitan OS"
                                    font { pixelSize: 11; family: "Inter" }
                                    font.weight: Font.Medium
                                    color: "#f2f4f8"
                                }
                                Text {
                                    text: "Settings v1.0"
                                    font { pixelSize: 10; family: "Inter" }
                                    color: "#9aa2b1"
                                }
                            }
                        }
                    }
                }
            }

            // ── Content area ──────────────────────────────────────
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // ── Title bar ─────────────────────────────────────
                Rectangle {
                    id: titleBar
                    anchors { top: parent.top; left: parent.left; right: parent.right }
                    height: 52
                    color: "transparent"

                    RowLayout {
                        anchors { fill: parent; leftMargin: 28; rightMargin: 18 }

                        Column {
                            spacing: 2
                            Text {
                                text: root.pages[root.currentPage].label
                                font { pixelSize: 17; family: "Inter" }
                                font.weight: Font.DemiBold
                                color: root.textHigh
                            }
                        }

                        Item { Layout.fillWidth: true }

                        // Window controls
                        RowLayout {
                            spacing: 8

                            Repeater {
                                model: [
                                    { col: "#ED6A5E", hov: "#C9504A", act: "close",    sym: "×" },
                                    { col: "#F5BF4F", hov: "#D4A030", act: "minimize", sym: "−" },
                                    { col: "#61C554", hov: "#48A83D", act: "maximize", sym: "+" }
                                ]
                                delegate: Item {
                                    width: 13; height: 13
                                    Rectangle {
                                        anchors.fill: parent; radius: 7
                                        color: wc.containsMouse ? modelData.hov : modelData.col
                                        Behavior on color { ColorAnimation { duration: 80 } }
                                    }
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.sym
                                        font.pixelSize: 9
                                        font.weight: Font.Bold
                                        color: "#00000070"
                                        visible: wc.containsMouse
                                    }
                                    MouseArea {
                                        id: wc; anchors.fill: parent; hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            if (modelData.act === "close")    root.close()
                                            else if (modelData.act === "minimize") root.showMinimized()
                                            else root.showMaximized()
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                        height: 1
                        color: root.border1
                    }
                }

                // ── Page stack ────────────────────────────────────
                Item {
                    anchors { top: titleBar.bottom; bottom: parent.bottom; left: parent.left; right: parent.right }
                    clip: true

                    StackLayout {
                        anchors.fill: parent
                        currentIndex: root.currentPage

                        AppearancePage { id: appearancePage }
                        DisplayPage    { id: displayPage    }
                        NetworkPage    { id: networkPage    }
                        AudioPage      { id: audioPage      }
                        PowerPage      { id: powerPage      }
                        SecurityPage   { id: securityPage   }
                        SystemPage     { id: systemPage     }
                        AboutPage      { id: aboutPage      }
                    }
                }
            }
        }
    }

    // ── Power profile switch toast overlay ─────────────────────────────────
    property var _profileMeta: {
        var p = SettingsBackend.powerProfile;
        if (p === "Performance")
            return { icon: "qrc:/ArchTitanSettings/assets/icons/performance_nobg.png",
                     label: "Performance", accent: "#E05C6A", colorize: false }
        if (p === "Power Saver")
            return { icon: "qrc:/ArchTitanSettings/assets/icons/powersaving.png",
                     label: "Power Saver", accent: "#4CAF82", colorize: false }
        return { icon: "qrc:/ArchTitanSettings/assets/icons/balanced.png",
                 label: "Balanced", accent: "#7AA2F7", colorize: false }
    }

    Item {
        id: profileToast
        anchors.fill: parent
        opacity: 0
        z: 999
        visible: opacity > 0

        // Backdrop tint
        Rectangle {
            anchors.fill: parent
            color: isDarkTheme ? "#88000000" : "#88FFFFFF"
            radius: 12
        }

        // Central card
        Rectangle {
            anchors.centerIn: parent
            width: 240; height: 220
            radius: 24
            color: isDarkTheme ? "#CC111111" : "#CCF5F5F5"
            border.width: 1
            border.color: root._profileMeta.accent

            // Outer glow ring
            Rectangle {
                anchors.centerIn: parent
                width: parent.width + 6; height: parent.height + 6
                radius: parent.radius + 3
                color: "transparent"
                border.width: 2
                border.color: Qt.rgba(
                    Qt.color(root._profileMeta.accent).r,
                    Qt.color(root._profileMeta.accent).g,
                    Qt.color(root._profileMeta.accent).b,
                    0.35)
                z: -1
            }

            Column {
                anchors.centerIn: parent
                spacing: 18

                // Icon container — pops in large
                Rectangle {
                    id: iconBox
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 120; height: 120; radius: 28
                    color: Qt.rgba(
                        Qt.color(root._profileMeta.accent).r,
                        Qt.color(root._profileMeta.accent).g,
                        Qt.color(root._profileMeta.accent).b,
                        0.18)
                    border.width: 1
                    border.color: Qt.rgba(
                        Qt.color(root._profileMeta.accent).r,
                        Qt.color(root._profileMeta.accent).g,
                        Qt.color(root._profileMeta.accent).b,
                        0.5)
                    scale: 0.1   // starts tiny, springs up via iconPopAnim

                    SequentialAnimation {
                        id: iconPopAnim
                        NumberAnimation {
                            target: iconBox; property: "scale"
                            from: 0.1; to: 1.12
                            duration: 280; easing.type: Easing.OutBack; easing.overshoot: 1.8
                        }
                        NumberAnimation {
                            target: iconBox; property: "scale"
                            to: 1.0; duration: 120; easing.type: Easing.InOutQuad
                        }
                    }

                    Image {
                        anchors.centerIn: parent
                        width: 88; height: 88
                        source: root._profileMeta.icon
                        fillMode: Image.PreserveAspectFit
                        smooth: true; mipmap: true
                    }
                }

                // Label
                Column {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 4

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "PROFILE SWITCHED"
                        font { pixelSize: 9; family: "Inter" }
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.6
                        color: isDarkTheme ? "#606060" : "#888888"
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: root._profileMeta.label
                        font { pixelSize: 20; family: "Inter" }
                        font.weight: Font.Bold
                        color: root._profileMeta.accent
                    }
                }
            }
        }

        // Fade-in → hold → fade-out
        SequentialAnimation {
            id: toastAnim
            NumberAnimation { target: profileToast; property: "opacity"; to: 1.0; duration: 220; easing.type: Easing.OutCubic }
            PauseAnimation  { duration: 2200 }
            NumberAnimation { target: profileToast; property: "opacity"; to: 0.0; duration: 380; easing.type: Easing.InCubic }
        }
    }

    // Watch for profile changes and trigger toast + icon pop
    Connections {
        target: SettingsBackend
        function onPowerProfileChanged() {
            toastAnim.stop()
            iconPopAnim.stop()
            profileToast.opacity = 0
            iconBox.scale = 0.1
            toastAnim.start()
            iconPopAnim.start()
        }
    }
}

