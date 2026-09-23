import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

RowLayout {
    id: headerRoot
    Layout.fillWidth: true
    Layout.preferredHeight: 38
    spacing: 8

    // ArchTitan Logo + Uptime Badge
    Rectangle {
        Layout.preferredHeight: 34
        Layout.preferredWidth: 106
        radius: 17
        color: "#1C202C"
        border.color: "#20FFFFFF"
        border.width: 1

        RowLayout {
            anchors.centerIn: parent
            spacing: 7

            // ArchTitan Delta Icon
            Canvas {
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                onPaint: {
                    var ctx = getContext("2d");
                    ctx.clearRect(0, 0, width, height);
                    ctx.beginPath();
                    ctx.moveTo(width * 0.5, 2);
                    ctx.lineTo(width - 2, height - 2);
                    ctx.lineTo(width * 0.65, height - 2);
                    ctx.lineTo(width * 0.5, height * 0.52);
                    ctx.lineTo(width * 0.35, height - 2);
                    ctx.lineTo(2, height - 2);
                    ctx.closePath();
                    ctx.fillStyle = "#38BDF8";
                    ctx.fill();
                }
            }

            Text {
                text: (typeof systemCtrl !== "undefined" && systemCtrl) ? systemCtrl.uptime : "Up 25m"
                font.family: "Outfit, Inter, sans-serif"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                color: "#FFFFFF"
            }
        }
    }

    Item { Layout.fillWidth: true }

    // Header Action Buttons
    RowLayout {
        spacing: 6

        // Edit Button
        Rectangle {
            width: 32; height: 32; radius: 16
            color: editMa.containsMouse ? "#2A3042" : "#1A1D27"
            Text { anchors.centerIn: parent; text: "✎"; color: "#94A3B8"; font.pixelSize: 14 }
            MouseArea { id: editMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor }
        }

        // Refresh Button
        Rectangle {
            width: 32; height: 32; radius: 16
            color: refreshMa.containsMouse ? "#2A3042" : "#1A1D27"
            Text { anchors.centerIn: parent; text: "↻"; color: "#94A3B8"; font.pixelSize: 15 }
            MouseArea {
                id: refreshMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (typeof systemCtrl !== "undefined" && systemCtrl) systemCtrl.refresh();
                    if (typeof networkCtrl !== "undefined" && networkCtrl) networkCtrl.refresh();
                    if (typeof bluetoothCtrl !== "undefined" && bluetoothCtrl) bluetoothCtrl.refresh();
                    if (typeof audioCtrl !== "undefined" && audioCtrl) audioCtrl.refresh();
                }
            }
        }

        // Settings Button
        Rectangle {
            width: 32; height: 32; radius: 16
            color: setMa.containsMouse ? "#2A3042" : "#1A1D27"
            Text { anchors.centerIn: parent; text: "⚙"; color: "#94A3B8"; font.pixelSize: 15 }
            MouseArea {
                id: setMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: if (typeof systemCtrl !== "undefined" && systemCtrl) systemCtrl.openSettings()
            }
        }

        // Power Button
        Rectangle {
            width: 32; height: 32; radius: 16
            color: pwrMa.containsMouse ? "#3A2024" : "#1A1D27"
            Text { anchors.centerIn: parent; text: "⏻"; color: pwrMa.containsMouse ? "#F87171" : "#94A3B8"; font.pixelSize: 15 }
            MouseArea {
                id: pwrMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: if (typeof systemCtrl !== "undefined" && systemCtrl) systemCtrl.openPowerMenu()
            }
        }
    }
}
