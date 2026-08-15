import QtQuick
import QtQuick.Layouts
import ArchTitanSettings

Rectangle {
    id: cardRoot
    color: globalBg3
    radius: 10
    border.width: 1
    border.color: hover.containsMouse ? globalBorder0 : globalBorder1
    implicitHeight: col.implicitHeight + 36
    clip: true

    Behavior on border.color { ColorAnimation { duration: 150 } }

    default property alias content: col.children
    property string title: ""
    property string headerIcon: ""

    ColumnLayout {
        id: col
        anchors { fill: parent; margins: 20 }
        spacing: 0

        // Optional title
        Item {
            visible: cardRoot.title !== ""
            Layout.fillWidth: true
            height: 28

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: cardRoot.title.toUpperCase()
                font { pixelSize: 10; family: "Inter" }
                font.weight: Font.DemiBold
                font.letterSpacing: 1.2
                color: globalTextLow
            }
        }

        // Subtle divider under title
        Rectangle {
            visible: cardRoot.title !== ""
            Layout.fillWidth: true
            height: 1
            color: globalBorder1
            Layout.bottomMargin: 12
        }
    }

    MouseArea {
        id: hover
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }
}
