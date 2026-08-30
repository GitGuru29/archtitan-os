import QtQuick

/**
 * TransportButton — Reusable circular control button.
 */
Item {
    id: btnRoot

    property int size: 28
    property string iconText: "▶"
    property int iconSize: 12
    property bool isPrimary: false
    property bool isActive: false

    signal activated()

    width: size
    height: size

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: width / 2

        color: {
            if (isPrimary) {
                return btnArea.containsMouse ? "#FFFFFF" : "#89B4FA"
            }
            if (isActive) {
                return btnArea.containsMouse ? "#3089B4FA" : "#2089B4FA"
            }
            return btnArea.containsMouse ? "#20FFFFFF" : "#10FFFFFF"
        }

        scale: btnArea.pressed ? 0.85 : (btnArea.containsMouse ? 1.06 : 1.0)

        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }
        Behavior on color { ColorAnimation { duration: 120 } }

        Text {
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: (iconText === "▶") ? 1 : 0
            text: btnRoot.iconText
            font.pixelSize: btnRoot.iconSize
            font.weight: Font.Bold
            color: {
                if (isPrimary) return "#11111B"
                if (isActive) return "#89B4FA"
                return btnArea.containsMouse ? "#CDD6F4" : "#A6ADC8"
            }
        }

        MouseArea {
            id: btnArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: btnRoot.activated()
        }
    }
}
