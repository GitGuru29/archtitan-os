import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

import "components"
import "style"

Window {
    id: rootWindow
    title: "titan-quicksettings"
    width: 420
    height: Screen.desktopAvailableHeight > 0 ? Math.min(Screen.desktopAvailableHeight - 24, 980) : 920

    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.BypassWindowManagerHint | Qt.Tool
    color: "transparent"

    x: Screen.width > 0 ? Screen.width - width - 12 : 1480
    y: 12

    property bool drawerOpen: false
    property bool drawerVisible: false

    visible: drawerVisible

    function openDrawer() {
        drawerVisible = true;
        drawerOpen = true;
        rootWindow.requestActivate();
    }

    function closeDrawer() {
        drawerOpen = false;
        closeTimer.start();
    }

    function toggleDrawer() {
        if (drawerOpen) {
            closeDrawer();
        } else {
            openDrawer();
        }
    }

    Timer {
        id: closeTimer
        interval: 280
        onTriggered: {
            if (!drawerOpen) {
                drawerVisible = false;
            }
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
        width: parent.width
        height: parent.height
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

        // Top Glass Highlight Line
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 1
            color: "#25FFFFFF"
            radius: 22
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            // 1. TOP HEADER BAR
            HeaderBar {
                systemCtrl: typeof systemCtrl !== "undefined" ? systemCtrl : null
                networkCtrl: typeof networkCtrl !== "undefined" ? networkCtrl : null
                bluetoothCtrl: typeof bluetoothCtrl !== "undefined" ? bluetoothCtrl : null
                audioCtrl: typeof audioCtrl !== "undefined" ? audioCtrl : null
            }

            // 2. QUICK SETTINGS CONTROLS GRID
            QuickToggleGrid {
                systemCtrl: typeof systemCtrl !== "undefined" ? systemCtrl : null
                networkCtrl: typeof networkCtrl !== "undefined" ? networkCtrl : null
                bluetoothCtrl: typeof bluetoothCtrl !== "undefined" ? bluetoothCtrl : null
                audioCtrl: typeof audioCtrl !== "undefined" ? audioCtrl : null
                nightLightCtrl: typeof nightLightCtrl !== "undefined" ? nightLightCtrl : null
                notifServer: typeof notifServer !== "undefined" ? notifServer : null
            }

            // 3. GROUPED NOTIFICATIONS LIST
            NotificationList {
                notifServer: typeof notifServer !== "undefined" ? notifServer : null
            }

            // 4. NOTIFICATION FOOTER
            NotificationFooter {
                notifServer: typeof notifServer !== "undefined" ? notifServer : null
            }

            // 5. BOTTOM CALENDAR & TASK BAR
            CalendarFooter {
                systemCtrl: typeof systemCtrl !== "undefined" ? systemCtrl : null
            }
        }
    }
}
