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
    color: Maui.Theme.backgroundColor

    // Layer Shell: Makes this window behave like a greeter
    LayerShell {
        id: layerShell
        window: root
    }

    // Show window after layer shell is activated to prevent flicker
    Component.onCompleted: {
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

    // Background gradient
    Rectangle {
        anchors.fill: parent
        color: Maui.Theme.backgroundColor

        Rectangle {
            anchors.fill: parent
            opacity: 0.3
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.lighter(Maui.Theme.backgroundColor, 1.1) }
                GradientStop { position: 1.0; color: Qt.darker(Maui.Theme.backgroundColor, 1.1) }
            }
        }
    }

    // Top Bar (Clock & Session)
    RowLayout {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Maui.Style.space.big
        spacing: Maui.Style.space.medium

        Label {
            id: clock
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
            model: sessionModel
            textRole: "name"
            implicitWidth: 200
        }
    }

    // Center Stage (Login Flow)
    StackLayout {
        id: loginStack
        anchors.centerIn: parent
        width: 400
        currentIndex: 0

        // View 0: Select User
        ColumnLayout {
            spacing: Maui.Style.space.big

            Maui.SectionHeader {
                Layout.fillWidth: true
                text1: "Select User"
                text2: "Choose your account to login"
            }

            ListView {
                id: userView
                Layout.preferredWidth: 400
                Layout.preferredHeight: 160
                orientation: ListView.Horizontal
                model: userModel
                spacing: Maui.Style.space.big
                clip: true

                delegate: Maui.ListBrowserDelegate {
                    width: 100
                    height: 140

                    iconSource: "user-identity"
                    iconSizeHint: Maui.Style.iconSizes.huge
                    label1.text: realName
                    label1.font.pixelSize: Maui.Style.fontSizes.small

                    isCurrentItem: ListView.isCurrentItem

                    onClicked: {
                        userView.currentIndex = index
                        auth.login(username)
                    }

                    // Custom avatar overlay (on top of the icon)
                    Item {
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: -10
                        width: Maui.Style.iconSizes.huge
                        height: Maui.Style.iconSizes.huge
                        z: 10

                        Image {
                            id: avatarImg
                            anchors.fill: parent
                            source: iconPath ? "file://" + iconPath : ""
                            fillMode: Image.PreserveAspectCrop
                            visible: false
                            cache: false
                        }

                        OpacityMask {
                            anchors.fill: avatarImg
                            source: avatarImg
                            maskSource: Rectangle {
                                width: Maui.Style.iconSizes.huge
                                height: Maui.Style.iconSizes.huge
                                radius: Maui.Style.iconSizes.huge / 2
                            }
                            visible: iconPath && avatarImg.status === Image.Ready
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

            Button {
                Layout.alignment: Qt.AlignHCenter
                text: "Cancel"

                onClicked: {
                    auth.cancel()
                    loginStack.currentIndex = 0
                }
            }
        }
    }

    // Bottom Controls (Power buttons)
    RowLayout {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: Maui.Style.space.big
        spacing: Maui.Style.space.medium

        Button {
            text: "Sleep"
            icon.name: "system-suspend"
            visible: true
            onClicked: power.suspend()
        }

        Button {
            text: "Reboot"
            icon.name: "system-reboot"
            visible: power.canReboot()
            onClicked: power.reboot()
        }

        Button {
            text: "Shutdown"
            icon.name: "system-shutdown"
            visible: power.canPowerOff()
            onClicked: power.powerOff()
        }
    }
}
