import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: pillRoot
    Layout.fillWidth: true
    Layout.preferredHeight: 46
    radius: 23

    property bool active: false
    property string iconText: "📶"
    property string titleText: "Tile"
    property string statusText: "Status"
    signal clicked()

    color: active ? "#38BDF8" : "#1F2330"
    border.color: active ? "#38BDF8" : "#20FFFFFF"
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 10

        // Circular Icon Container
        Rectangle {
            width: 28
            height: 28
            radius: 14
            color: pillRoot.active ? "#200C1524" : "#2E3444"

            Text {
                anchors.centerIn: parent
                text: pillRoot.iconText
                font.pixelSize: 13
                color: pillRoot.active ? "#0A1322" : "#FFFFFF"
            }
        }

        // Title and Status Labels
        ColumnLayout {
            spacing: 0

            Text {
                text: pillRoot.titleText
                font.family: "Outfit, Inter, sans-serif"
                font.pixelSize: 13
                font.weight: Font.Bold
                color: pillRoot.active ? "#0A1322" : "#FFFFFF"
            }

            Text {
                text: pillRoot.statusText
                font.family: "Outfit, Inter, sans-serif"
                font.pixelSize: 11
                color: pillRoot.active ? "#1A324E" : "#94A3B8"
                elide: Text.ElideRight
                Layout.maximumWidth: 100
            }
        }

        Item { Layout.fillWidth: true }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: pillRoot.clicked()
    }
}
