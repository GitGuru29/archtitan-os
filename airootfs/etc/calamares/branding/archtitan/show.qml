import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Presentation {
    id: presentation

    Timer {
        interval: 5000
        running: true
        repeat: true
        onTriggered: presentation.goToNextSlide()
    }

    // Slide 1: Welcome
    Slide {
        Rectangle {
            anchors.fill: parent
            color: "#1A1B26"

            Column {
                anchors.centerIn: parent
                spacing: 24
                width: parent.width * 0.8

                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    source: "logo.png"
                    width: 120
                    height: 120
                    fillMode: Image.PreserveAspectFit
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Welcome to ArchTitan OS"
                    font.pointSize: 22
                    font.weight: Font.Bold
                    color: "#C0CAF5"
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "A powerful, developer-focused Arch Linux distribution\nbuilt for performance and elegance."
                    font.pointSize: 12
                    color: "#A9B1D6"
                    horizontalAlignment: Text.AlignHCenter
                    lineHeight: 1.5
                    wrapMode: Text.WordWrap
                    width: parent.width
                }
            }
        }
    }

    // Slide 2: Hyprland Desktop
    Slide {
        Rectangle {
            anchors.fill: parent
            color: "#1A1B26"

            Column {
                anchors.centerIn: parent
                spacing: 24
                width: parent.width * 0.8

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 80
                    height: 80
                    radius: 40
                    color: "#24283B"
                    border.width: 2
                    border.color: "#7AA2F7"

                    Text {
                        anchors.centerIn: parent
                        text: "🖥"
                        font.pointSize: 32
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Hyprland Desktop Environment"
                    font.pointSize: 20
                    font.weight: Font.Bold
                    color: "#C0CAF5"
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Experience a blazing-fast Wayland compositor with\nsmooth animations, dynamic tiling, and full GPU acceleration.\n\n• Waybar status bar with real-time system monitoring\n• Wofi application launcher\n• Kitty GPU-accelerated terminal"
                    font.pointSize: 11
                    color: "#A9B1D6"
                    horizontalAlignment: Text.AlignHCenter
                    lineHeight: 1.5
                    wrapMode: Text.WordWrap
                    width: parent.width
                }
            }
        }
    }

    // Slide 3: Developer Tools
    Slide {
        Rectangle {
            anchors.fill: parent
            color: "#1A1B26"

            Column {
                anchors.centerIn: parent
                spacing: 24
                width: parent.width * 0.8

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 80
                    height: 80
                    radius: 40
                    color: "#24283B"
                    border.width: 2
                    border.color: "#9ECE6A"

                    Text {
                        anchors.centerIn: parent
                        text: "⚡"
                        font.pointSize: 32
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Developer Powerhouse"
                    font.pointSize: 20
                    font.weight: Font.Bold
                    color: "#C0CAF5"
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Pre-configured with a complete development environment\nready to code from the moment you log in.\n\n• VS Code, Neovim & full toolchain\n• Node.js, Go, Rust & Docker\n• Git, GitHub CLI & Zellij multiplexer"
                    font.pointSize: 11
                    color: "#A9B1D6"
                    horizontalAlignment: Text.AlignHCenter
                    lineHeight: 1.5
                    wrapMode: Text.WordWrap
                    width: parent.width
                }
            }
        }
    }

    // Slide 4: Security & Performance
    Slide {
        Rectangle {
            anchors.fill: parent
            color: "#1A1B26"

            Column {
                anchors.centerIn: parent
                spacing: 24
                width: parent.width * 0.8

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 80
                    height: 80
                    radius: 40
                    color: "#24283B"
                    border.width: 2
                    border.color: "#BB9AF7"

                    Text {
                        anchors.centerIn: parent
                        text: "🛡"
                        font.pointSize: 32
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Built for Performance"
                    font.pointSize: 20
                    font.weight: Font.Bold
                    color: "#C0CAF5"
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "ArchTitan ships with optimized defaults that balance\nperformance, security, and usability.\n\n• Rolling release — always up to date\n• Minimal bloat — only what you need\n• Full disk encryption support (LUKS)"
                    font.pointSize: 11
                    color: "#A9B1D6"
                    horizontalAlignment: Text.AlignHCenter
                    lineHeight: 1.5
                    wrapMode: Text.WordWrap
                    width: parent.width
                }
            }
        }
    }
}
