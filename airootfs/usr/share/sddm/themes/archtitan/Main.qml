import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15
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
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatTime(new Date(), "HH:mm")
            font.pointSize: 64
            font.weight: Font.Light
            color: "#C0CAF5"
            font.family: "JetBrainsMono Nerd Font"
        }

        Text {
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
        interval: 30000
        running: true
        repeat: true
        onTriggered: { }
    }

    // Login Card (Glassmorphic)
    Rectangle {
        id: loginCard
        anchors.centerIn: parent
        width: 380
        height: 380
        radius: 20
        color: "#24283B"
        opacity: 0.85
        border.width: 1
        border.color: "#414868"

        // Glow effect behind card
        layer.enabled: true
        layer.effect: DropShadow {
            transparentBorder: true
            horizontalOffset: 0
            verticalOffset: 4
            radius: 24
            samples: 49
            color: "#407AA2F7"
        }

        Column {
            anchors.centerIn: parent
            spacing: 20
            width: parent.width - 60

            // Logo
            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: "logo.png"
                width: 80
                height: 80
                fillMode: Image.PreserveAspectFit
            }

            // Welcome text
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Welcome to ArchTitan"
                color: "#C0CAF5"
                font.pointSize: 14
                font.weight: Font.DemiBold
                font.family: "JetBrainsMono Nerd Font"
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

            // Login Button
            Button {
                id: loginButton
                width: parent.width
                height: 44
                text: "Login"
                font.pointSize: 12
                font.weight: Font.DemiBold
                font.family: "JetBrainsMono Nerd Font"

                contentItem: Text {
                    text: loginButton.text
                    font: loginButton.font
                    color: "#1A1B26"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 10
                    color: loginButton.pressed ? "#5D7CE0" : (loginButton.hovered ? "#8BAFFA" : "#7AA2F7")

                    Behavior on color {
                        ColorAnimation { duration: 150 }
                    }
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
