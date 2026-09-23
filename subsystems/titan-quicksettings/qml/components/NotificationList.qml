import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: notifListRoot
    Layout.fillWidth: true
    Layout.fillHeight: true

    ListView {
        id: notifListView
        anchors.fill: parent
        spacing: 8
        clip: true
        model: (typeof notifServer !== "undefined" && notifServer) ? notifServer.notifications : []

        delegate: NotificationCard {
            modelData: modelData
            cardIndex: index
        }

        // Empty State if all notifications cleared
        Rectangle {
            anchors.centerIn: parent
            width: parent.width - 40
            height: 80
            radius: 14
            color: "#12151E"
            visible: notifListView.count === 0

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 4
                Text { text: "🔔"; font.pixelSize: 22; Layout.alignment: Qt.AlignHCenter }
                Text { text: "No notifications — all caught up"; font.pixelSize: 12; color: "#64748B"; Layout.alignment: Qt.AlignHCenter }
            }
        }
    }
}
