import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: cardRoot
    width: parent ? parent.width : 390
    height: expanded ? 120 : 66
    radius: 14

    required property var modelData
    required property int cardIndex
    required property var notifServer

    property bool expanded: modelData ? (modelData.expanded || false) : false

    color: cardMa.containsMouse ? "#1E2230" : "#171A24"
    border.color: cardMa.containsMouse ? "#3038BDF8" : "#14FFFFFF"
    border.width: 1

    Behavior on height {
        NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        // Circular App Icon Badge
        Rectangle {
            width: 36; height: 36; radius: 18
            color: (cardRoot.modelData.iconType === "antigravity") ? "#111420" : "#0284C7"
            border.color: (cardRoot.modelData.iconType === "antigravity") ? "#38BDF8" : "transparent"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: {
                    if (cardRoot.modelData.iconType === "wifi") return "📶";
                    if (cardRoot.modelData.iconType === "antigravity") return "▲";
                    if (cardRoot.modelData.iconType === "terminal") return "💻";
                    return "💬";
                }
                font.pixelSize: 14
                color: (cardRoot.modelData.iconType === "antigravity") ? "#38BDF8" : "#FFFFFF"
                font.bold: true
            }
        }

        // Notification Content
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            // Header row: App Name, Date, Count Badge
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: cardRoot.modelData.appName
                    font.family: "Outfit, Inter, sans-serif"
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                    color: "#CBD5E1"
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: cardRoot.modelData.dateStr
                    font.family: "Outfit, Inter, sans-serif"
                    font.pixelSize: 11
                    color: "#64748B"
                }

                // Expandable Count Badge
                Rectangle {
                    height: 18
                    width: countText.contentWidth + 14
                    radius: 9
                    color: "#242838"
                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 2
                        Text {
                            id: countText
                            text: cardRoot.modelData.count + ""
                            font.pixelSize: 10
                            font.weight: Font.Bold
                            color: "#94A3B8"
                        }
                        Text {
                            text: cardRoot.expanded ? "▴" : "▾"
                            font.pixelSize: 9
                            color: "#94A3B8"
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (cardRoot.notifServer) cardRoot.notifServer.toggleExpanded(cardRoot.cardIndex)
                    }
                }
            }

            // Primary Text
            Text {
                text: cardRoot.modelData.primaryText
                font.family: "Outfit, Inter, sans-serif"
                font.pixelSize: 12
                font.weight: Font.Bold
                color: "#FFFFFF"
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            // Secondary Text
            Text {
                text: cardRoot.modelData.secondaryText
                font.family: "Outfit, Inter, sans-serif"
                font.pixelSize: 11
                color: "#94A3B8"
                elide: cardRoot.expanded ? Text.ElideNone : Text.ElideRight
                wrapMode: cardRoot.expanded ? Text.Wrap : Text.NoWrap
                Layout.fillWidth: true
            }
        }

        // Dismiss button on hover
        Rectangle {
            width: 22; height: 22; radius: 11
            color: dismissMa.containsMouse ? "#471D22" : "transparent"
            visible: cardMa.containsMouse
            Text { anchors.centerIn: parent; text: "×"; color: "#F87171"; font.pixelSize: 14; font.bold: true }
            MouseArea {
                id: dismissMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: if (cardRoot.notifServer) cardRoot.notifServer.dismissAt(cardRoot.cardIndex)
            }
        }
    }

    MouseArea {
        id: cardMa
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true
        onClicked: if (cardRoot.notifServer) cardRoot.notifServer.toggleExpanded(cardRoot.cardIndex)
    }
}
