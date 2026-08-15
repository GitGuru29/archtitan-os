import QtQuick
import QtQuick.Controls.Basic
import ArchTitanSettings

Slider {
    id: root
    property color trackColor: globalBorder0
    property color fillColor:  "#4C8BF5"

    implicitHeight: 20

    background: Item {
        implicitHeight: 20

        // Track
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width; height: 4; radius: 2
            color: root.trackColor
            border.width: 1
            border.color: globalBorder1
        }

        // Fill
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: root.visualPosition * parent.width
            height: 4; radius: 2
            color: root.fillColor
        }
    }

    handle: Rectangle {
        x: root.leftPadding + root.visualPosition * (root.availableWidth - width)
        y: root.topPadding + root.availableHeight / 2 - height / 2
        width: 16; height: 16; radius: 8
        color: root.fillColor
        border.width: 0

        scale: root.pressed ? 0.9 : 1.0
        Behavior on scale { NumberAnimation { duration: 80 } }
    }
}
