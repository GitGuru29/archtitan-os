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

    Behavior on color { ColorAnimation { duration: 150 } }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 8

        // Circular Icon Container
        Rectangle {
            width: 30
            height: 30
            radius: 15
            color: pillRoot.active ? "#250A1322" : "#2A3040"

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
            Layout.fillWidth: true

            Text {
                text: pillRoot.titleText
                font.family: "Outfit, Inter, sans-serif"
                font.pixelSize: 12
                font.weight: Font.Bold
                color: pillRoot.active ? "#0A1322" : "#FFFFFF"
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Text {
                text: pillRoot.statusText
                font.family: "Outfit, Inter, sans-serif"
                font.pixelSize: 10
                color: pillRoot.active ? "#15304B" : "#94A3B8"
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: pillRoot.clicked()
    }
}
