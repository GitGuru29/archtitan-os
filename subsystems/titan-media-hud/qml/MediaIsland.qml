import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import ArchTitan.Media 1.0

Window {
    id: root
    title: "titan-media-hud"
    width: 460
    height: islandContent.implicitHeight
    color: "transparent"
    visible: false

    // ─── CONSTANTS ────────────────────────────────────────────────────────────
    readonly property int compactHeight: 58
    readonly property int expandedHeight: 210
    readonly property color bgColor: "#EB11111B"   // Catppuccin Crust 92%
    readonly property color borderColor: "#2489B4FA" // Sapphire 14%
    readonly property color accentPrimary: "#89B4FA"
    readonly property color accentSecondary: "#74C7EC"
    readonly property color textPrimary: "#CDD6F4"
    readonly property color textSecondary: "#A6ADC8"
    readonly property color textDim: "#585B70"
    readonly property color surfaceColor: "#1E1E2E"

    // ─── KEYBOARD HANDLING ────────────────────────────────────────────────────
    Item {
        focus: Island.isVisible
        Keys.onEscapePressed: Island.dismiss()
    }

    // ─── CLIP CONTAINER (emergence effect — clips from top) ───────────────────
    Item {
        id: clipContainer
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        width: 460
        height: islandContent.implicitHeight
        clip: true

        // The actual island body
        Rectangle {
            id: islandContent
            width: 460
            implicitHeight: (Island.isExpanded || Island.isExpanding)
                            ? expandedHeight : compactHeight
            anchors.horizontalCenter: parent.horizontalCenter
            y: 0

            radius: 18
            color: root.bgColor
            border.color: root.borderColor
            border.width: 1

            // ── VISUAL POLISH ──────────────────────────────────────────────────
            // Top specular highlight
            Rectangle {
                anchors.top: parent.top
                anchors.topMargin: 1
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width * 0.6
                height: 1
                radius: 1
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "transparent" }
                    GradientStop { position: 0.3; color: "#30FFFFFF" }
                    GradientStop { position: 0.7; color: "#30FFFFFF" }
                    GradientStop { position: 1.0; color: "transparent" }
                }
            }

            // Soft shadow
            Rectangle {
                anchors.fill: parent
                anchors.margins: -6
                radius: parent.radius + 6
                color: "transparent"
                z: -1
                Rectangle {
                    anchors.fill: parent
                    radius: parent.radius
                    color: "#40000000"
                }
            }

            Behavior on implicitHeight {
                NumberAnimation {
                    duration: 300
                    easing.type: Easing.OutCubic
                }
            }

            // ── CONTENT STACK ──────────────────────────────────────────────────
            CompactView {
                id: compactView
                anchors.fill: parent
                anchors.margins: 8
                visible: opacity > 0
                opacity: (Island.isCompact || Island.isOpening || Island.isClosing) ? 1.0 : 0.0

                Behavior on opacity {
                    NumberAnimation { duration: 200; easing.type: Easing.OutQuad }
                }

                onExpandRequested: Island.expand()
            }

            ExpandedView {
                id: expandedView
                anchors.fill: parent
                anchors.margins: 12
                visible: opacity > 0
                opacity: (Island.isExpanded || Island.isExpanding) ? 1.0 : 0.0

                Behavior on opacity {
                    NumberAnimation { duration: 250; easing.type: Easing.OutQuad }
                }

                onCollapseRequested: Island.collapse()
            }

            // ── NO MEDIA STATE ─────────────────────────────────────────────────
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 4
                visible: !(Mpris && Mpris.hasMedia) && Island.isVisible
                opacity: visible ? 0.6 : 0.0

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "♫"
                    font.pixelSize: 20
                    color: root.textDim
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "No media playing"
                    font.family: "JetBrainsMono Nerd Font"
                    font.pixelSize: 11
                    color: root.textDim
                }
            }
        }
    }

    // ─── INITIAL HIDDEN STATE ─────────────────────────────────────────────────
    // Animations are driven imperatively by the Island state machine.
    // Start hidden (opacity 0, scaled down).
    Component.onCompleted: {
        clipContainer.opacity = 0.0
        islandContent.scale = 0.7
    }

    // ── OPEN ANIMATION ─────────────────────────────────────────────────────
    ParallelAnimation {
        id: openAnim

        NumberAnimation {
            target: clipContainer
            property: "opacity"
            from: 0.0; to: 1.0
            duration: 120
            easing.type: Easing.OutQuad
        }

        NumberAnimation {
            target: islandContent
            property: "scale"
            from: 0.7; to: 1.0
            duration: 380
            easing.type: Easing.OutBack
            easing.overshoot: 1.05
        }

        NumberAnimation {
            target: islandContent
            property: "y"
            from: -compactHeight * 0.6; to: 0
            duration: 350
            easing.type: Easing.OutBack
            easing.overshoot: 1.02
        }

        onFinished: Island.onOpenAnimationFinished()
    }

    // ── CLOSE ANIMATION ────────────────────────────────────────────────────
    ParallelAnimation {
        id: closeAnim

        NumberAnimation {
            target: clipContainer
            property: "opacity"
            from: 1.0; to: 0.0
            duration: 200
            easing.type: Easing.InQuad
        }

        NumberAnimation {
            target: islandContent
            property: "scale"
            from: 1.0; to: 0.75
            duration: 280
            easing.type: Easing.InBack
            easing.overshoot: 0.8
        }

        NumberAnimation {
            target: islandContent
            property: "y"
            from: 0; to: -compactHeight * 0.5
            duration: 250
            easing.type: Easing.InCubic
        }

        onFinished: {
            Island.onCloseAnimationFinished()
            // Reset transforms
            islandContent.y = 0
            islandContent.scale = 0.7
        }
    }

    // ── EXPAND ANIMATION ───────────────────────────────────────────────────
    SequentialAnimation {
        id: expandAnim
        onFinished: Island.onExpandAnimationFinished()
    }

    // ── COLLAPSE ANIMATION ─────────────────────────────────────────────────
    SequentialAnimation {
        id: collapseAnim
        onFinished: Island.onCollapseAnimationFinished()
    }

    // ── STATE MACHINE → ANIMATION ROUTING ──────────────────────────────────
    Connections {
        target: Island

        function onRequestOpen() {
            closeAnim.stop()
            islandContent.y = -compactHeight * 0.6
            islandContent.scale = 0.7
            clipContainer.opacity = 0.0
            openAnim.start()
        }

        function onRequestClose() {
            openAnim.stop()
            closeAnim.start()
        }

        function onRequestExpand() {
            // Height transition handled by Behavior on implicitHeight
            Island.onExpandAnimationFinished()
        }

        function onRequestCollapse() {
            // Height transition handled by Behavior on implicitHeight
            Island.onCollapseAnimationFinished()
        }
    }

    // ── CLICK OUTSIDE TO DISMISS ───────────────────────────────────────────
    // (Layer shell surfaces don't receive mouse events outside their bounds,
    //  so this is not strictly needed, but ESC works as the primary dismiss)
}
