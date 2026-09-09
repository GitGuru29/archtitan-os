import QtQuick
import QtQuick.Layouts
import ArchTitan.Media 1.0

/**
 * ActiveWindowPage — Active Application & Focused Window Tracker
 * Displays active Hyprland window title, application class, and workspace context.
 */
Item {
    id: windowPageRoot
    anchors.fill: parent

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        anchors.topMargin: 6
        anchors.bottomMargin: 6
        spacing: 14

        // ── APP ICON BADGE ───────────────────────────────────────────────
        Rectangle {
            Layout.preferredWidth: 46
            Layout.preferredHeight: 46
            radius: 12
            color: "#1889B4FA"
            border.color: "#4089B4FA"
            border.width: 1.5

            Text {
                anchors.centerIn: parent
                text: "</>"
                font.family: "JetBrainsMono Nerd Font"
                font.pixelSize: 18
                font.weight: Font.Bold
                color: "#89B4FA"
            }

            Rectangle {
                anchors.fill: parent
                anchors.margins: -3
                radius: parent.radius + 3
                color: "transparent"
                border.color: "#3089B4FA"
                border.width: 1
            }
        }

        // ── DETAILS ──────────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 2

            RowLayout {
                spacing: 6

                Rectangle {
                    height: 14
                    width: tagText.implicitWidth + 10
                    radius: 7
                    color: "#2089B4FA"
                    border.color: "#4089B4FA"
                    border.width: 1

                    Text {
                        id: tagText
                        anchors.centerIn: parent
                        text: "ACTIVE WINDOW"
                        font.family: "JetBrainsMono Nerd Font"
                        font.pixelSize: 8
                        font.weight: Font.Bold
                        font.letterSpacing: 0.8
                        color: "#89B4FA"
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: System ? System.activeWindowTitle : "VS Code — main.cpp"
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 13
                    font.weight: Font.Bold
                    color: "#FFFFFF"
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
            }

            Text {
                Layout.fillWidth: true
                text: System ? System.activeWindowApp : "ArchTitan Development"
                font.family: "JetBrainsMono Nerd Font"
                font.pixelSize: 11
                color: "#A6ADC8"
                elide: Text.ElideRight
            }
        }
    }
}
