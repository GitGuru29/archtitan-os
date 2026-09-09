import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import ArchTitanSettings

Item {
    id: root
    height: 44
    implicitHeight: 44

    property string iconSource: ""
    property string label: ""
    property bool   active: false

    signal clicked()

    // Background pill
    Rectangle {
        anchors {
            fill: parent
            leftMargin: 12
            rightMargin: 12
        }
        radius: 9
        color: root.active 
               ? Qt.rgba(0.298, 0.545, 0.96, 0.16)
               : (hover.containsMouse ? Qt.rgba(1, 1, 1, 0.05) : "transparent")
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    RowLayout {
        anchors {
            fill: parent
            leftMargin: 24
            rightMargin: 24
        }
        spacing: 14

        // Fixed 19px icon container for perfect vertical column alignment
        Item {
            Layout.preferredWidth: 19
            Layout.preferredHeight: 19

            Image {
                id: iconImg
                anchors.fill: parent
                source: root.iconSource
                fillMode: Image.PreserveAspectFit
                smooth: true
                visible: false
            }

            MultiEffect {
                anchors.fill: iconImg
                source: iconImg
                colorization: 1.0
                colorizationColor: root.active 
                                   ? "#7cb0ff" 
                                   : (hover.containsMouse ? "#d0d5e0" : "#9aa2b1")
                Behavior on colorizationColor { ColorAnimation { duration: 120 } }
            }
        }

        Text {
            text: root.label
            font {
                pixelSize: 13
                weight: root.active ? Font.Medium : Font.Normal
                family: "Inter"
            }
            color: root.active 
                   ? "#f2f4f8" 
                   : (hover.containsMouse ? "#d0d5e0" : "#9aa2b1")
            Behavior on color { ColorAnimation { duration: 120 } }
            Layout.fillWidth: true
        }
    }

    MouseArea {
        id: hover
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
