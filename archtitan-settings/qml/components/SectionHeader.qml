import QtQuick
import QtQuick.Layouts
import ArchTitanSettings

Item {
    property string text: ""
    height: 32
    Layout.fillWidth: true

    Row {
        anchors { left: parent.left; verticalCenter: parent.verticalCenter }
        spacing: 10

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: parent.parent.text.toUpperCase()
            font { pixelSize: 10; family: "Inter" }
            font.weight: Font.DemiBold
            font.letterSpacing: 1.2
            color: "#4A4A4A"
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: {
                var pw = parent.parent.parent.width
                var tw = parent.parent.text.length * 8 + 16
                return Math.max(0, pw - tw - 16)
            }
            height: 1
            color: "#222222"
            visible: parent.parent.text !== ""
        }
    }
}
