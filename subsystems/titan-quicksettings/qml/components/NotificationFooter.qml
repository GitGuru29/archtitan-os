import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

RowLayout {
    id: footerRoot
    Layout.fillWidth: true
    Layout.preferredHeight: 38
    spacing: 8

    required property var notifServer

    // DND Toggle
    Rectangle {
        width: 36; height: 36; radius: 18
        color: (footerRoot.notifServer && footerRoot.notifServer.dndActive) ? "#38BDF8" : "#1A1D27"
        Text { anchors.centerIn: parent; text: "🔕"; font.pixelSize: 14 }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: if (footerRoot.notifServer) footerRoot.notifServer.toggleDnd()
        }
    }

    // Total Notifications Pill
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 36
        radius: 18
        color: "#1A1D27"
        Text {
            anchors.centerIn: parent
            text: footerRoot.notifServer ? (footerRoot.notifServer.totalCount + " notifications") : "8967 notifications"
            font.family: "Outfit, Inter, sans-serif"
            font.pixelSize: 12
            font.weight: Font.DemiBold
            color: "#94A3B8"
        }
    }

    // Clear All / Trash Button
    Rectangle {
        width: 36; height: 36; radius: 18
        color: trashMa.containsMouse ? "#3A2024" : "#1A1D27"
        Text { anchors.centerIn: parent; text: "🗑"; font.pixelSize: 15; color: trashMa.containsMouse ? "#F87171" : "#94A3B8" }
        MouseArea {
            id: trashMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: if (footerRoot.notifServer) footerRoot.notifServer.clearAll()
        }
    }
}
