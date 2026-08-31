import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import ArchTitan.Media 1.0

import "pages"

Window {
    id: root
    title: "titan-media-hud"
    width: 540
    height: islandContent.implicitHeight + 12
    color: "transparent"
    visible: false

    // ─── CONSTANTS ────────────────────────────────────────────────────────────
    readonly property int hudWidth: 540
    readonly property int hudHeight: 76
    readonly property color bgColor: "#F011111B"       // Deep Catppuccin Mocha Crust 94% glass

    // ─── KEYBOARD DISMISS (Escape Key) ────────────────────────────────────────
    Item {
        focus: Island.isVisible
        Keys.onEscapePressed: Island.dismiss()
    }

    // ─── MASTER PHYSICAL HUD CONTAINER ───────────────────────────────────────
    Item {
        id: clipContainer
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        width: root.hudWidth
        height: root.hudHeight + 10
        clip: false

        Rectangle {
            id: islandContent
            width: root.hudWidth
            implicitHeight: root.hudHeight
            anchors.horizontalCenter: parent.horizontalCenter
            y: 0

            radius: 22
            color: root.bgColor
            border.color: getDynamicBorderColor()
            border.width: 1.2

            Behavior on border.color { ColorAnimation { duration: 300 } }

            // Dynamic border color depending on active page
            function getDynamicBorderColor() {
                var p = System ? System.currentPageName : "media"
                if (p === "power") return "#4074C7EC"
                if (p === "window") return "#4089B4FA"
                if (p === "temperature") return "#40FAB387"
                if (p === "datetime") return "#40A6E3A1"
                if (p === "battery") return "#40F9E2AF"
                if (p === "gpu") return "#40CBA6F7"
                return "#4089B4FA" // default sapphire for media
            }

            // ── TOP SPECULAR GLASS HIGHLIGHT ──────────────────────────────────
            Rectangle {
                anchors.top: parent.top
                anchors.topMargin: 1
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width * 0.72
                height: 1
                radius: 1
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "transparent" }
                    GradientStop { position: 0.3; color: "#35FFFFFF" }
                    GradientStop { position: 0.7; color: "#35FFFFFF" }
                    GradientStop { position: 1.0; color: "transparent" }
                }
            }

            // ── SOFT DROP SHADOW ──────────────────────────────────────────────
            Rectangle {
                anchors.fill: parent
                anchors.margins: -4
                radius: parent.radius + 4
                color: "transparent"
                z: -1
                border.color: "#30000000"
                border.width: 4
            }

            // ── PAGINATION INDICATOR DOTS ─────────────────────────────────────
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                anchors.topMargin: 5
                spacing: 5
                z: 10
                visible: System ? System.pageCount > 1 : false

                Repeater {
                    model: System ? System.pageCount : 1

                    Rectangle {
                        width: (index === (System ? System.currentPageIndex : 0)) ? 14 : 5
                        height: 5
                        radius: 2.5
                        color: (index === (System ? System.currentPageIndex : 0)) ? "#89B4FA" : "#40FFFFFF"

                        Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutQuad } }
                        Behavior on color { ColorAnimation { duration: 200 } }
                    }
                }
            }

            // ── MOUSE WHEEL & GESTURE INTERACTION ─────────────────────────────
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onWheel: (wheel) => {
                    if (System) {
                        System.userScrolled(wheel.angleDelta.y)
                    }
                }
            }

            // ── VERTICAL PAGE CAROUSEL VIEWPORT ───────────────────────────────
            Item {
                id: carouselViewport
                anchors.fill: parent
                anchors.topMargin: 6
                clip: true

                // Track previous index to determine scroll direction (up/down)
                property int lastIndex: 0
                property bool scrollDown: true

                Connections {
                    target: System
                    function onCurrentPageIndexChanged() {
                        var newIdx = System.currentPageIndex
                        carouselViewport.scrollDown = (newIdx >= carouselViewport.lastIndex) ||
                                                     (carouselViewport.lastIndex === System.pageCount - 1 && newIdx === 0)
                        carouselViewport.lastIndex = newIdx
                    }
                }

                // 1. Media Page
                MediaPage {
                    visible: opacity > 0.01
                    opacity: (System && System.currentPageName === "media") ? 1.0 : 0.0
                    y: (System && System.currentPageName === "media") ? 0 :
                       (carouselViewport.scrollDown ? -parent.height : parent.height)

                    Behavior on opacity { NumberAnimation { duration: 280; easing.type: Easing.OutQuad } }
                    Behavior on y { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
                }

                // 2. Power Profile Page
                PowerPage {
                    visible: opacity > 0.01
                    opacity: (System && System.currentPageName === "power") ? 1.0 : 0.0
                    y: (System && System.currentPageName === "power") ? 0 :
                       (carouselViewport.scrollDown ? -parent.height : parent.height)

                    Behavior on opacity { NumberAnimation { duration: 280; easing.type: Easing.OutQuad } }
                    Behavior on y { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
                }

                // 3. Active Window Page
                ActiveWindowPage {
                    visible: opacity > 0.01
                    opacity: (System && System.currentPageName === "window") ? 1.0 : 0.0
                    y: (System && System.currentPageName === "window") ? 0 :
                       (carouselViewport.scrollDown ? -parent.height : parent.height)

                    Behavior on opacity { NumberAnimation { duration: 280; easing.type: Easing.OutQuad } }
                    Behavior on y { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
                }

                // 4. CPU Temperature Page
                TemperaturePage {
                    visible: opacity > 0.01
                    opacity: (System && System.currentPageName === "temperature") ? 1.0 : 0.0
                    y: (System && System.currentPageName === "temperature") ? 0 :
                       (carouselViewport.scrollDown ? -parent.height : parent.height)

                    Behavior on opacity { NumberAnimation { duration: 280; easing.type: Easing.OutQuad } }
                    Behavior on y { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
                }

                // 5. Date & Time Page
                DateTimePage {
                    visible: opacity > 0.01
                    opacity: (System && System.currentPageName === "datetime") ? 1.0 : 0.0
                    y: (System && System.currentPageName === "datetime") ? 0 :
                       (carouselViewport.scrollDown ? -parent.height : parent.height)

                    Behavior on opacity { NumberAnimation { duration: 280; easing.type: Easing.OutQuad } }
                    Behavior on y { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
                }

                // 6. Battery Page
                BatteryPage {
                    visible: opacity > 0.01
                    opacity: (System && System.currentPageName === "battery") ? 1.0 : 0.0
                    y: (System && System.currentPageName === "battery") ? 0 :
                       (carouselViewport.scrollDown ? -parent.height : parent.height)

                    Behavior on opacity { NumberAnimation { duration: 280; easing.type: Easing.OutQuad } }
                    Behavior on y { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
                }

                // 7. GPU Page
                GpuPage {
                    visible: opacity > 0.01
                    opacity: (System && System.currentPageName === "gpu") ? 1.0 : 0.0
                    y: (System && System.currentPageName === "gpu") ? 0 :
                       (carouselViewport.scrollDown ? -parent.height : parent.height)

                    Behavior on opacity { NumberAnimation { duration: 280; easing.type: Easing.OutQuad } }
                    Behavior on y { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
                }
            }
        }
    }

    // ─── INITIAL HIDDEN STATE ─────────────────────────────────────────────────
    Component.onCompleted: {
        clipContainer.opacity = 0.0
        islandContent.scale = 0.75
    }

    // ─── OPEN ANIMATION (Emerges from Waybar) ─────────────────────────────────
    ParallelAnimation {
        id: openAnim

        NumberAnimation {
            target: clipContainer
            property: "opacity"
            from: 0.0; to: 1.0
            duration: 150
            easing.type: Easing.OutQuad
        }

        NumberAnimation {
            target: islandContent
            property: "scale"
            from: 0.75; to: 1.0
            duration: 360
            easing.type: Easing.OutBack
            easing.overshoot: 1.06
        }

        NumberAnimation {
            target: islandContent
            property: "y"
            from: -root.hudHeight * 0.7; to: 0
            duration: 340
            easing.type: Easing.OutBack
            easing.overshoot: 1.04
        }

        onFinished: Island.onOpenAnimationFinished()
    }

    // ─── CLOSE ANIMATION (Retracts to Waybar) ─────────────────────────────────
    ParallelAnimation {
        id: closeAnim

        NumberAnimation {
            target: clipContainer
            property: "opacity"
            from: 1.0; to: 0.0
            duration: 180
            easing.type: Easing.InQuad
        }

        NumberAnimation {
            target: islandContent
            property: "scale"
            from: 1.0; to: 0.75
            duration: 240
            easing.type: Easing.InBack
            easing.overshoot: 0.8
        }

        NumberAnimation {
            target: islandContent
            property: "y"
            from: 0; to: -root.hudHeight * 0.5
            duration: 220
            easing.type: Easing.InCubic
        }

        onFinished: {
            Island.onCloseAnimationFinished()
            islandContent.y = 0
            islandContent.scale = 0.75
        }
    }

    // ─── STATE MACHINE ROUTING ────────────────────────────────────────────────
    Connections {
        target: Island

        function onRequestOpen() {
            closeAnim.stop()
            islandContent.y = -root.hudHeight * 0.7
            islandContent.scale = 0.75
            clipContainer.opacity = 0.0
            openAnim.start()
        }

        function onRequestClose() {
            openAnim.stop()
            closeAnim.start()
        }

        function onRequestExpand() {
            Island.onExpandAnimationFinished()
        }

        function onRequestCollapse() {
            Island.onCollapseAnimationFinished()
        }
    }
}
