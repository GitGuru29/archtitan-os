import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import ArchTitan.Media 1.0

Window {
    id: root
    title: "titan-media-hud"
    width: 490
    height: islandContent.implicitHeight + 12
    color: "transparent"
    visible: false

    // ─── CONSTANTS ────────────────────────────────────────────────────────────
    readonly property int islandWidth: 490
    readonly property int compactHeight: 56
    readonly property int expandedHeight: 220
    readonly property color bgColor: "#F011111B"       // Catppuccin Mocha Crust 94% glass
    readonly property color borderColor: "#2889B4FA"   // Sapphire 16% neon border
    readonly property color accentPrimary: "#89B4FA"
    readonly property color accentSecondary: "#74C7EC"
    readonly property color textPrimary: "#CDD6F4"
    readonly property color textSecondary: "#A6ADC8"
    readonly property color textDim: "#585B70"

    // ─── KEYBOARD ESCAPE LISTENER ─────────────────────────────────────────────
    Item {
        focus: Island.isVisible
        Keys.onEscapePressed: Island.dismiss()
    }

    // ─── CLIP CONTAINER (Dynamic emergence from top edge) ────────────────────
    Item {
        id: clipContainer
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        width: root.islandWidth
        height: islandContent.implicitHeight + 10
        clip: false

        // The island pill body
        Rectangle {
            id: islandContent
            width: root.islandWidth
            implicitHeight: (Island.isExpanded || Island.isExpanding)
                            ? root.expandedHeight : root.compactHeight
            anchors.horizontalCenter: parent.horizontalCenter
            y: 0

            radius: 20
            color: root.bgColor
            border.color: root.borderColor
            border.width: 1

            // ── TOP SPECULAR GLASS HIGHLIGHT ──────────────────────────────────
            Rectangle {
                anchors.top: parent.top
                anchors.topMargin: 1
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width * 0.7
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

            Behavior on implicitHeight {
                NumberAnimation {
                    duration: 320
                    easing.type: Easing.OutCubic
                }
            }

            // ── COMPACT VIEW ──────────────────────────────────────────────────
            CompactView {
                id: compactView
                anchors.fill: parent
                anchors.margins: 4
                visible: opacity > 0.01
                opacity: (Island.isCompact || Island.isOpening || Island.isClosing) ? 1.0 : 0.0

                Behavior on opacity {
                    NumberAnimation { duration: 180; easing.type: Easing.OutQuad }
                }

                onExpandRequested: Island.expand()
            }

            // ── EXPANDED VIEW ─────────────────────────────────────────────────
            ExpandedView {
                id: expandedView
                anchors.fill: parent
                anchors.margins: 10
                visible: opacity > 0.01
                opacity: (Island.isExpanded || Island.isExpanding) ? 1.0 : 0.0

                Behavior on opacity {
                    NumberAnimation { duration: 240; easing.type: Easing.OutQuad }
                }

                onCollapseRequested: Island.collapse()
            }

            // ── NO MEDIA IDLE STATE ───────────────────────────────────────────
            RowLayout {
                anchors.centerIn: parent
                spacing: 8
                visible: !(Mpris && Mpris.hasMedia) && Island.isVisible
                opacity: visible ? 0.7 : 0.0

                Text {
                    text: "♫"
                    font.pixelSize: 16
                    color: root.accentPrimary
                }
                Text {
                    text: "No Media Playing"
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 11
                    font.weight: Font.Medium
                    color: root.textSecondary
                }
            }
        }
    }

    // ─── INITIAL HIDDEN STATE ─────────────────────────────────────────────────
    Component.onCompleted: {
        clipContainer.opacity = 0.0
        islandContent.scale = 0.75
    }

    // ─── OPEN ANIMATION ───────────────────────────────────────────────────────
    ParallelAnimation {
        id: openAnim

        NumberAnimation {
            target: clipContainer
            property: "opacity"
            from: 0.0; to: 1.0
            duration: 140
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
            from: -root.compactHeight * 0.7; to: 0
            duration: 340
            easing.type: Easing.OutBack
            easing.overshoot: 1.04
        }

        onFinished: Island.onOpenAnimationFinished()
    }

    // ─── CLOSE ANIMATION ──────────────────────────────────────────────────────
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
            from: 0; to: -root.compactHeight * 0.5
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
            islandContent.y = -root.compactHeight * 0.7
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
