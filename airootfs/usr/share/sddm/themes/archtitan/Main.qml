import QtQuick 2.15
import QtQuick.Controls 2.15
import Qt5Compat.GraphicalEffects
import SddmComponents 2.0

Rectangle {
    id: root
    width: 1920
    height: 1080
    color: "#1A1B26"

    property string defaultUsername: ""

    TextConstants { id: textConstants }

    // Background Image
    Image {
        id: backgroundImage
        anchors.fill: parent
        source: "background.png"
        fillMode: Image.PreserveAspectCrop
    }

    // Background Blur
    FastBlur {
        anchors.fill: backgroundImage
        source: backgroundImage
        radius: 40
    }

    // Dark Overlay
    Rectangle {
        anchors.fill: parent
        color: "#1A1B26"
        opacity: 0.55
    }

    // Clock Display
    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: parent.height * 0.08
        spacing: 4

        Text {
            id: clockTime
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatTime(new Date(), "HH:mm")
            font.pointSize: 64
            font.weight: Font.Light
            color: "#C0CAF5"
            font.family: "JetBrainsMono Nerd Font"
        }

        Text {
            id: clockDate
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatDate(new Date(), "dddd, MMMM d yyyy")
            font.pointSize: 14
            font.weight: Font.Light
            color: "#565F89"
            font.family: "JetBrainsMono Nerd Font"
        }
    }

    // Timer for clock updates
    Timer {
        interval: 10000
        running: true
        repeat: true
        onTriggered: {
            clockTime.text = Qt.formatTime(new Date(), "HH:mm")
            clockDate.text = Qt.formatDate(new Date(), "dddd, MMMM d yyyy")
        }
    }

    // Floating Animation for the Card
    SequentialAnimation {
        running: true
        loops: Animation.Infinite
        NumberAnimation { target: loginCard; property: "anchors.verticalCenterOffset"; to: -5; duration: 2000; easing.type: Easing.InOutQuad }
        NumberAnimation { target: loginCard; property: "anchors.verticalCenterOffset"; to: 5; duration: 2000; easing.type: Easing.InOutQuad }
    }

    // Login Card (Ultra Glassmorphic)
    Rectangle {
        id: loginCard
        anchors.centerIn: parent
        width: 420
        height: 440
        radius: 24
        color: "#151828"
        opacity: 0.75
        border.width: 1
        border.color: "#3B4261"

        // Animated Glow effect behind card
        layer.enabled: true
        layer.effect: DropShadow {
            transparentBorder: true
            horizontalOffset: 0
            verticalOffset: 10
            radius: 35
            samples: 71
            color: "#607AA2F7"
        }

        Column {
            anchors.centerIn: parent
            spacing: 20
            width: parent.width - 60

            // Logo with rotation hover effect
            Image {
                id: brandLogo
                anchors.horizontalCenter: parent.horizontalCenter
                source: "logo.png"
                width: 100
                height: 100
                fillMode: Image.PreserveAspectFit
                
                Behavior on scale { NumberAnimation { duration: 300; easing.type: Easing.OutBack } }
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: brandLogo.scale = 1.1
                    onExited: brandLogo.scale = 1.0
                }
            }

            // Welcome text with gradient appearance
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "ARCHTITAN OS"
                color: "#FFFFFF"
                font.pointSize: 18
                font.weight: Font.Bold
                font.letterSpacing: 2
                font.family: "JetBrainsMono Nerd Font"
                
                layer.enabled: true
                layer.effect: DropShadow {
                    horizontalOffset: 0
                    verticalOffset: 0
                    radius: 10
                    samples: 21
                    color: "#7AA2F7"
                }
            }

            // Username Field
            TextField {
                id: usernameField
                width: parent.width
                height: 44
                placeholderText: "Username"
                text: userModel.lastUser
                font.pointSize: 11
                font.family: "JetBrainsMono Nerd Font"
                color: "#C0CAF5"

                background: Rectangle {
                    radius: 10
                    color: "#1A1B26"
                    border.width: 2
                    border.color: usernameField.activeFocus ? "#7AA2F7" : "#414868"

                    Behavior on border.color {
                        ColorAnimation { duration: 200 }
                    }
                }

                Keys.onReturnPressed: passwordField.focus = true
            }

            // Password Field
            TextField {
                id: passwordField
                width: parent.width
                height: 44
                placeholderText: "Password"
                echoMode: TextInput.Password
                font.pointSize: 11
                font.family: "JetBrainsMono Nerd Font"
                color: "#C0CAF5"

                background: Rectangle {
                    radius: 10
                    color: "#1A1B26"
                    border.width: 2
                    border.color: passwordField.activeFocus ? "#7AA2F7" : "#414868"

                    Behavior on border.color {
                        ColorAnimation { duration: 200 }
                    }
                }

                Keys.onReturnPressed: sddm.login(usernameField.text, passwordField.text, sessionModel.lastIndex)
            }

            // Login Button (Premium Gradient)
            Button {
                id: loginButton
                width: parent.width
                height: 48
                font.pointSize: 13
                font.weight: Font.Bold
                font.family: "JetBrainsMono Nerd Font"

                contentItem: Text {
                    text: "INITIALIZE SESSION"
                    font: loginButton.font
                    color: "#FFFFFF"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.letterSpacing: 1
                }

                background: Rectangle {
                    id: btnBg
                    radius: 12
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: loginButton.pressed ? "#3D59A1" : (loginButton.hovered ? "#7AA2F7" : "#2AC3DE") }
                        GradientStop { position: 1.0; color: loginButton.pressed ? "#2AC3DE" : (loginButton.hovered ? "#BB9AF7" : "#7AA2F7") }
                    }
                    
                    // Button glow
                    layer.enabled: loginButton.hovered
                    layer.effect: DropShadow {
                        transparentBorder: true
                        horizontalOffset: 0
                        verticalOffset: 2
                        radius: 15
                        samples: 31
                        color: "#807AA2F7"
                    }

                    Behavior on scale { NumberAnimation { duration: 200; easing.type: Easing.OutBack } }
                }

                // Qt6: onPressedChanged / onHoveredChanged removed — use onPressed/onReleased instead
                onPressed:  btnBg.scale = 0.95
                onReleased: btnBg.scale = loginButton.hovered ? 1.02 : 1.0
                onCanceled: btnBg.scale = 1.0
                HoverHandler {
                    onHoveredChanged: btnBg.scale = hovered ? 1.02 : 1.0
                }
                onClicked: sddm.login(usernameField.text, passwordField.text, sessionModel.lastIndex)
            }
        }
    }

    // Error Message
    Text {
        id: errorMessage
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: loginCard.bottom
        anchors.topMargin: 16
        text: ""
        color: "#F7768E"
        font.pointSize: 11
        font.family: "JetBrainsMono Nerd Font"
    }

    // Session Selector (bottom left)
    Row {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 20
        spacing: 8

        Text {
            text: "Session:"
            color: "#565F89"
            font.pointSize: 10
            font.family: "JetBrainsMono Nerd Font"
            anchors.verticalCenter: parent.verticalCenter
        }

        ComboBox {
            id: sessionSelector
            width: 160
            height: 32
            model: sessionModel
            currentIndex: sessionModel.lastIndex
            textRole: "name"
            font.pointSize: 10
            font.family: "JetBrainsMono Nerd Font"

            contentItem: Text {
                text: sessionSelector.displayText
                color: "#A9B1D6"
                font: sessionSelector.font
                verticalAlignment: Text.AlignVCenter
                leftPadding: 10
            }

            background: Rectangle {
                radius: 8
                color: "#24283B"
                border.color: "#414868"
                border.width: 1
            }

            popup.background: Rectangle {
                color: "#24283B"
                border.color: "#414868"
                radius: 8
            }

            delegate: ItemDelegate {
                width: sessionSelector.width
                contentItem: Text {
                    text: model.name
                    color: "#A9B1D6"
                    font.pointSize: 10
                    font.family: "JetBrainsMono Nerd Font"
                }
                background: Rectangle {
                    color: highlighted ? "#7AA2F7" : "transparent"
                    radius: 4
                }
                highlighted: sessionSelector.highlightedIndex === index
            }
        }
    }

    // Power Buttons (bottom right)
    Row {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20
        spacing: 12

        // Restart button
        Rectangle {
            width: 36
            height: 36
            radius: 18
            color: hoverRestart.hovered ? "#414868" : "#24283B"
            border.width: 1
            border.color: "#414868"

            Text {
                anchors.centerIn: parent
                text: "⟳"
                color: "#A9B1D6"
                font.pointSize: 16
            }

            HoverHandler { id: hoverRestart }

            MouseArea {
                anchors.fill: parent
                onClicked: sddm.reboot()
                cursorShape: Qt.PointingHandCursor
            }

            Behavior on color { ColorAnimation { duration: 150 } }
        }

        // Shutdown button
        Rectangle {
            width: 36
            height: 36
            radius: 18
            color: hoverShutdown.hovered ? "#414868" : "#24283B"
            border.width: 1
            border.color: "#414868"

            Text {
                anchors.centerIn: parent
                text: "⏻"
                color: "#A9B1D6"
                font.pointSize: 16
            }

            HoverHandler { id: hoverShutdown }

            MouseArea {
                anchors.fill: parent
                onClicked: sddm.powerOff()
                cursorShape: Qt.PointingHandCursor
            }

            Behavior on color { ColorAnimation { duration: 150 } }
        }
    }

    // Handle login errors
    Connections {
        target: sddm
        function onLoginFailed() {
            errorMessage.text = "Login failed. Please try again."
            passwordField.text = ""
            passwordField.focus = true
        }
        function onLoginSucceeded() {
            errorMessage.text = ""
        }
    }

    // Focus username on load
    Component.onCompleted: {
        if (usernameField.text === "")
            usernameField.focus = true
        else
            passwordField.focus = true
    }
}
