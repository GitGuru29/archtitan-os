import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

import "components"
import "style"

Item {
    id: rootItem
    width: 420

    property bool drawerOpen: false

    function openDrawer() {
        drawerOpen = true;
    }

    function closeDrawer() {
        drawerOpen = false;
    }

    function toggleDrawer() {
        if (drawerOpen) {
            closeDrawer();
        } else {
            openDrawer();
        }
    }

    Item {
        anchors.fill: parent
        focus: true
        Keys.onEscapePressed: closeDrawer()
    }

    // Main Drawer Card Container with Slide Animation
    Rectangle {
        id: card
        anchors.fill: parent
        radius: 22
        color: "#F011141D" // 94% opacity deep obsidian
        border.color: "#2E38BDF8" // Subtle 1px cyan border
        border.width: 1
        clip: true

        // Slide animation using overshot easing
        x: drawerOpen ? 0 : width + 30
        Behavior on x {
            NumberAnimation {
                duration: 260
                easing.type: Easing.OutBack
                easing.overshoot: 1.05
            }
        }


        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            // 1. TOP HEADER BAR
            HeaderBar {}

            // 2. QUICK SETTINGS CONTROLS GRID
            QuickToggleGrid {}

            // 3. GROUPED NOTIFICATIONS LIST
            NotificationList {}

            // 4. NOTIFICATION FOOTER
            NotificationFooter {}

            // 5. BOTTOM CALENDAR & TASK BAR
            CalendarFooter {}
        }
    }
}
