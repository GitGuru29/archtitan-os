import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: cardRoot
    width: parent ? parent.width : 390
    height: expanded ? 116 : 68
    radius: 14

    property var modelItem: (typeof modelData !== "undefined") ? modelData : null
    property int cardIndex: (typeof index !== "undefined") ? index : 0

    readonly property string appNameStr: (modelItem && modelItem.appName) ? modelItem.appName : "Notification"
    readonly property string dateStrVal: (modelItem && modelItem.dateStr) ? modelItem.dateStr : ""
    readonly property int countVal: (modelItem && modelItem.count) ? modelItem.count : 1
    readonly property string primaryTextStr: (modelItem && modelItem.primaryText) ? modelItem.primaryText : ""
    readonly property string secondaryTextStr: (modelItem && modelItem.secondaryText) ? modelItem.secondaryText : ""
    readonly property string iconTypeStr: (modelItem && modelItem.iconType) ? modelItem.iconType : ""
    property bool expanded: modelItem ? (modelItem.expanded || false) : false

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
            color: (cardRoot.iconTypeStr === "antigravity") ? "#111420" :
                   (cardRoot.iconTypeStr === "wifi") ? "#0C2433" : "#0E2234"
            border.color: (cardRoot.iconTypeStr === "antigravity") ? "#38BDF8" : "#20FFFFFF"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: {
                    if (cardRoot.iconTypeStr === "wifi") return "📶";
                    if (cardRoot.iconTypeStr === "antigravity") return "▲";
                    if (cardRoot.iconTypeStr === "terminal") return "💻";
                    return "💬";
                }
                font.pixelSize: 14
                color: "#38BDF8"
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
                spacing: 6

                Text {
                    text: cardRoot.appNameStr
                    font.family: "Outfit, Inter, sans-serif"
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    color: "#CBD5E1"
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: cardRoot.dateStrVal
                    font.family: "Outfit, Inter, sans-serif"
                    font.pixelSize: 10
                    color: "#64748B"
                }

                // Expandable Count Badge
                Rectangle {
                    height: 18
                    width: countText.contentWidth + 16
                    radius: 9
                    color: "#242838"
                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 3
                        Text {
                            id: countText
                            text: cardRoot.countVal + ""
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
                        onClicked: if (typeof notifServer !== "undefined" && notifServer) notifServer.toggleExpanded(cardRoot.cardIndex)
                    }
                }
            }

            // Primary Text
            Text {
                text: cardRoot.primaryTextStr
                font.family: "Outfit, Inter, sans-serif"
                font.pixelSize: 12
                font.weight: Font.Bold
                color: "#FFFFFF"
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            // Secondary Text
            Text {
                text: cardRoot.secondaryTextStr
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
                onClicked: if (typeof notifServer !== "undefined" && notifServer) notifServer.dismissAt(cardRoot.cardIndex)
            }
        }
    }

    MouseArea {
        id: cardMa
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true
        onClicked: if (typeof notifServer !== "undefined" && notifServer) notifServer.toggleExpanded(cardRoot.cardIndex)
    }
}
