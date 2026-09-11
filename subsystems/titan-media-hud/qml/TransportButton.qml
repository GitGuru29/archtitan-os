import QtQuick

/**
 * TransportButton — Tactile, modern circular playback button with
 * hover glow, scale feedback, and vibrant primary action state.
 */
Item {
    id: btnRoot

    property int size: 28
    property string iconText: "▶"
    property int iconSize: 12
    property bool isPrimary: false
    property bool isActive: false
    property string tooltip: ""

    signal activated()

    width: size
    height: size

    Rectangle {
        id: bg
        anchors.fill: parent
        radius: width / 2

        // Gradient for primary (Play/Pause) vs sleek glass for others
        gradient: isPrimary ? primaryGradient : null
        color: {
            if (isPrimary) return "#89B4FA"
            if (isActive) return btnArea.containsMouse ? "#3589B4FA" : "#2089B4FA"
            return btnArea.containsMouse ? "#24FFFFFF" : "#12FFFFFF"
        }

        border.color: {
            if (isPrimary) return "#B4BEFE"
            if (isActive) return "#89B4FA"
            return btnArea.containsMouse ? "#40FFFFFF" : "#18FFFFFF"
        }
        border.width: 1

        Gradient {
            id: primaryGradient
            GradientStop { position: 0.0; color: btnArea.containsMouse ? "#B4BEFE" : "#89B4FA" }
            GradientStop { position: 1.0; color: btnArea.containsMouse ? "#89B4FA" : "#74C7EC" }
        }

        // Tactile press & hover scale
        scale: btnArea.pressed ? 0.88 : (btnArea.containsMouse ? 1.08 : 1.0)

        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
        Behavior on color { ColorAnimation { duration: 140 } }

        // Glow ring for primary
        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: width / 2
            color: "transparent"
            border.color: "#6089B4FA"
            border.width: 1.5
            visible: isPrimary && btnArea.containsMouse
            opacity: btnArea.containsMouse ? 0.8 : 0.0

            Behavior on opacity { NumberAnimation { duration: 150 } }
        }

        Text {
            id: iconLabel
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: (btnRoot.iconText === "▶") ? 1 : 0
            text: btnRoot.iconText
            font.family: "JetBrainsMono Nerd Font"
            font.pixelSize: btnRoot.iconSize
            font.weight: Font.Bold
            color: {
                if (isPrimary) return "#11111B" // Dark icon on bright sapphire
                if (isActive) return "#89B4FA"
                return btnArea.containsMouse ? "#FFFFFF" : "#CDD6F4"
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
