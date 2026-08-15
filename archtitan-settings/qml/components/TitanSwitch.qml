import QtQuick
import QtQuick.Controls.Basic
import ArchTitanSettings

Switch {
    id: root

    property color onColor:  "#4C8BF5"
    property color offColor: globalBorder0

    implicitWidth: 44
    implicitHeight: 24

    indicator: Rectangle {
        width: root.implicitWidth
        height: root.implicitHeight
        radius: height / 2
        color: root.checked ? root.onColor : root.offColor
        border.width: 1
        border.color: root.checked ? "transparent" : globalBorder1

        Behavior on color { ColorAnimation { duration: 180 } }

        // Knob
        Rectangle {
            width: 18; height: 18
            radius: 9
            color: "#FFFFFF"
            anchors.verticalCenter: parent.verticalCenter
            x: root.checked ? parent.width - width - 3 : 3
            Behavior on x { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

            // Subtle drop shadow imitation
            Rectangle {
                anchors { fill: parent; margins: 1 }
                radius: parent.radius
                color: "transparent"
                border.width: 1
                border.color: "#00000020"
            }
        }
    }

    contentItem: Item {}
}
