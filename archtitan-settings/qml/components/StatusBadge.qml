import QtQuick
import ArchTitanSettings

Rectangle {
    property string text: "Active"
    property color  statusColor: "#4CAF82"

    height: 20
    radius: 10
    width: lbl.implicitWidth + 20
    color: Qt.rgba(statusColor.r, statusColor.g, statusColor.b, 0.12)
    border.width: 1
    border.color: Qt.rgba(statusColor.r, statusColor.g, statusColor.b, 0.3)

    Row {
        anchors.centerIn: parent
        spacing: 6

        Rectangle {
            width: 6; height: 6; radius: 3
            color: parent.parent.statusColor
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            id: lbl
            text: parent.parent.text
            font { pixelSize: 11; family: "Inter" }
            font.weight: Font.Medium
            color: parent.parent.statusColor
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
