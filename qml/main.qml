import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import org.mauikit.controls as Maui
import QmlGreet 1.0

Window {
    id: root
    width: 1920
    height: 1080
    visible: false
    flags: Qt.FramelessWindowHint
    color: "transparent"

    // Layer Shell: Makes this window behave like a greeter
    LayerShell {
        id: layerShell
        window: root
    }

    // Window Blur Effect
    Maui.WindowBlur {
        view: root
        geometry: Qt.rect(0, 0, root.width, root.height)
        windowRadius: 0
        enabled: true
    }

    // Show window after layer shell is activated to prevent flicker
    Component.onCompleted: {
        console.log("=== QmlGreet Debug ===")
        console.log("Background image path:", ColorScheme.backgroundImage)
        layerShell.activate()
        root.visible = true
    }

    // Auth Wrapper: Handles talking to greetd
    AuthWrapper {
        id: auth

        onPromptChanged: {
            passwordField.text = ""
            passwordField.forceActiveFocus()
            loginStack.currentIndex = 1
        }

        onLoginSucceeded: {
            var cmd = sessionModel.execCommand(sessionCombo.currentIndex)
            console.log("Authentication success. Launching:", cmd)
            auth.startSession(cmd)
        }

        onErrorChanged: {
            if (auth.error !== "") {
                errorAnimation.start()
            }
        }
    }

    SystemPower { id: power }
    UserModel { id: userModel }
    SessionModel { id: sessionModel }

    // Background layer 1: Base color or image
    Rectangle {
        anchors.fill: parent
        color: Maui.Theme.backgroundColor
        z: 0

        // Background Image (if specified)
        Image {
            id: backgroundImage
            anchors.fill: parent
            source: (ColorScheme.backgroundImage && ColorScheme.backgroundImage !== "")
                    ? "file://" + ColorScheme.backgroundImage
                    : ""
            fillMode: Image.PreserveAspectCrop
            visible: false // Hidden because we show the blurred version
            cache: false

            Component.onCompleted: {
                console.log("ColorScheme.backgroundImage:", ColorScheme.backgroundImage)
                console.log("Background Image source:", source)
            }

            onStatusChanged: {
                console.log("Background Image status changed to:", status, "(1=Loading, 2=Ready, 3=Error)")
                if (status === Image.Error) {
                    console.log("Error loading background image!")
                }
                if (status === Image.Ready) {
                    console.log("Background image loaded successfully!")
                }
            }
        }

        // Blurred background image
        FastBlur {
            anchors.fill: parent
            source: backgroundImage
            radius: 64
            visible: backgroundImage.status === Image.Ready
            cached: true
        }

        // Gradient overlay (only if no background image)
        Rectangle {
            anchors.fill: parent
            opacity: 0.3
            visible: backgroundImage.status !== Image.Ready
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.lighter(Maui.Theme.backgroundColor, 1.1) }
                GradientStop { position: 1.0; color: Qt.darker(Maui.Theme.backgroundColor, 1.1) }
            }
        }
    }

    // Semi-transparent overlay for better contrast (always visible, on top of background)
    Rectangle {
        anchors.fill: parent
        color: Maui.Theme.backgroundColor
        opacity: 0.76
        z: 1
    }

    // Top Bar (Clock & Session) - Responsive layout
    ColumnLayout {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: Maui.Style.space.big
        spacing: Maui.Style.space.small
        z: 10
        width: Math.min(parent.width - Maui.Style.space.big * 2, 600)

        Label {
            id: clock
            Layout.alignment: Qt.AlignHCenter
            color: Maui.Theme.textColor
            font.pixelSize: Maui.Style.fontSizes.big
            font.bold: true

            Timer {
                interval: 1000
                running: true
                repeat: true
                onTriggered: clock.text = Qt.formatDateTime(new Date(), "hh:mm A - dddd, MMM d")
            }
        }

        ComboBox {
            id: sessionCombo
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(parent.width, 300)
            model: sessionModel
            textRole: "name"
        }
    }

    // Center Stage (Login Flow)
    StackLayout {
        id: loginStack
        anchors.centerIn: parent
        width: Math.min(parent.width - Maui.Style.space.big * 2, 600)
        currentIndex: 0
        z: 10

        // View 0: Select User
        ColumnLayout {
            spacing: Maui.Style.space.big

            Maui.SectionHeader {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                text1: "Select User"
                text2: "Choose your account to login"
            }

            // User avatar container - centered
            Item {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Math.min(parent.width, 500)
                Layout.preferredHeight: 220

                ListView {
                    id: userView
                    anchors.centerIn: parent
                    width: Math.min(parent.width, contentWidth)
                    height: 220
                    orientation: ListView.Horizontal
                    model: userModel
                    spacing: Maui.Style.space.big
                    clip: false
                    interactive: contentWidth > width

                    delegate: Item {
                        width: 140
                        height: 220

                        ColumnLayout {
                            anchors.centerIn: parent
                            spacing: Maui.Style.space.medium

                            // Avatar container with circular highlight
                            Rectangle {
                                Layout.alignment: Qt.AlignHCenter
                                width: 120
                                height: 120
                                radius: 60
                                color: "transparent"
                                border.color: ListView.isCurrentItem ? Maui.Theme.highlightColor : "transparent"
                                border.width: 3

                                // Base icon (hidden when avatar image is loaded)
                                Maui.Icon {
                                    anchors.centerIn: parent
                                    width: 112
                                    height: 112
                                    source: "user-identity"
                                    color: Maui.Theme.textColor
                                    visible: !avatarImg.visible
                                }

                                // Custom avatar overlay (circular)
                                Item {
                                    anchors.centerIn: parent
                                    width: 112
                                    height: 112

                                    Image {
                                        id: avatarImg
                                        anchors.fill: parent
                                        source: iconPath ? "file://" + iconPath : ""
                                        fillMode: Image.PreserveAspectCrop
                                        visible: false
                                        cache: false
                                    }

                                    OpacityMask {
                                        id: maskedAvatar
                                        anchors.fill: avatarImg
                                        source: avatarImg
                                        maskSource: Rectangle {
                                            width: 112
                                            height: 112
                                            radius: 56
                                        }
                                        visible: iconPath && avatarImg.status === Image.Ready
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        userView.currentIndex = index
                                        auth.login(username)
                                    }
                                }
                            }

                            // Username label below avatar
                            Label {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.preferredWidth: 130
                                text: realName
                                color: Maui.Theme.textColor
                                font.pixelSize: Maui.Style.fontSizes.medium
                                font.weight: Font.Medium
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                                wrapMode: Text.WordWrap
                                maximumLineCount: 2
                            }
                        }
                    }
                }
            }
        }

        // View 1: Password Entry
        ColumnLayout {
            spacing: Maui.Style.space.medium

            Maui.SectionHeader {
                Layout.fillWidth: true
                text1: auth.currentPrompt || "Password"
                text2: "Enter your password to continue"
            }

            Maui.TextField {
                id: passwordField
                Layout.fillWidth: true
                Layout.preferredHeight: Maui.Style.rowHeight

                echoMode: auth.isSecret ? TextInput.Password : TextInput.Normal
                placeholderText: "Enter password"

                onAccepted: auth.respond(text)

                // Shake animation on error
                SequentialAnimation {
                    id: errorAnimation
                    NumberAnimation { target: passwordField; property: "x"; to: passwordField.x + 10; duration: 50 }
                    NumberAnimation { target: passwordField; property: "x"; to: passwordField.x - 10; duration: 50 }
                    NumberAnimation { target: passwordField; property: "x"; to: passwordField.x; duration: 50 }
                }
            }

            Label {
                Layout.fillWidth: true
                text: auth.error
                color: Maui.Theme.negativeTextColor
                visible: auth.error !== ""
                font.pixelSize: Maui.Style.fontSizes.small
                horizontalAlignment: Text.AlignHCenter
            }

            StyledButton {
                Layout.alignment: Qt.AlignHCenter
                iconName: "go-previous"

                onClicked: {
                    auth.cancel()
                    loginStack.currentIndex = 0
                }
            }
        }
    }

    // Bottom floating bar with power buttons
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Maui.Style.space.small
        width: buttonRow.width + (Maui.Style.space.medium * 2)
        height: 56
        color: Maui.Theme.backgroundColor
        radius: Maui.Style.radiusV
        z: 10

        RowLayout {
            id: buttonRow
            anchors.centerIn: parent
            spacing: Maui.Style.space.small

            StyledButton {
                iconName: "system-suspend"
                visible: true
                onClicked: power.suspend()
            }

            StyledButton {
                iconName: "system-reboot"
                visible: power.canReboot()
                onClicked: power.reboot()
            }

            StyledButton {
                iconName: "system-shutdown"
                visible: power.canPowerOff()
                onClicked: power.powerOff()
            }
        }
    }
}
