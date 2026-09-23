import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: calFooterRoot
    Layout.fillWidth: true
    Layout.preferredHeight: 36
    radius: 18
    color: "#161924"
    border.color: "#14FFFFFF"
    border.width: 1

    required property var systemCtrl

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        spacing: 10

        Text {
            text: "▲"
            font.pixelSize: 10
            color: "#64748B"
        }

        Text {
            text: calFooterRoot.systemCtrl ? calFooterRoot.systemCtrl.currentDateString : "Thursday, September 17 • 0 tasks"
            font.family: "Outfit, Inter, sans-serif"
            font.pixelSize: 12
            font.weight: Font.DemiBold
            color: "#CBD5E1"
        }

        Item { Layout.fillWidth: true }
    }
}
