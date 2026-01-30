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

    LayerShell { id: layerShell; window: root }

    Maui.WindowBlur {
        view: root
        geometry: Qt.rect(0, 0, root.width, root.height)
        windowRadius: 0
        enabled: true
    }

    // Robust selection logic (Two-Pass)
    function selectDefaultSession() {
        if (sessionModel.rowCount() === 0) return;

        if (ConfigDefaultSession !== "") {
            // PASS 1: Strict exact match (Priority)
            for (var i = 0; i < sessionModel.rowCount(); i++) {
                var name = sessionModel.data(sessionModel.index(i, 0), 257)
                if (name === ConfigDefaultSession) {
                    sessionCombo.currentIndex = i
                    console.log("Selected Default Session (Exact):", name)
                    return
                }
            }

            // PASS 2: Fuzzy/partial match (Fallback)
            for (var j = 0; j < sessionModel.rowCount(); j++) {
                var partialName = sessionModel.data(sessionModel.index(j, 0), 257)
                if (partialName.indexOf(ConfigDefaultSession) !== -1) {
                    sessionCombo.currentIndex = j
                    console.log("Selected Default Session (Partial):", partialName)
                    return
                }
            }
        }

        // Fallback: Select first item if nothing else worked
        if (sessionCombo.currentIndex < 0) {
            sessionCombo.currentIndex = 0
        }
    }

    Component.onCompleted: {
        layerShell.activate()
        root.visible = true
        if (userModel.rowCount() > 0) userCombo.currentIndex = 0
        
        selectDefaultSession()
    }

    Connections {
        target: sessionModel
        function onRowsInserted() { selectDefaultSession() }
        function onModelReset() { selectDefaultSession() }
    }

    AuthWrapper {
        id: auth
        onPromptChanged: {
            passwordField.text = ""
            passwordField.forceActiveFocus()
            loginStack.currentIndex = 1
        }
        onLoginSucceeded: {
            auth.error = ""

            // Ensure valid selection before launch
            if (sessionCombo.currentIndex < 0) {
                selectDefaultSession()
            }

            var idx = sessionCombo.currentIndex
            if (idx >= 0) {
                var cmd = sessionModel.execCommand(idx)
                console.log("Launching session:", cmd)
                auth.startSession(cmd)
            } else {
                auth.error = "No session selected"
            }
        }
        onErrorChanged: { if (auth.error !== "") errorAnimation.start() }
    }

    SystemPower { id: power }
    SystemBattery { id: battery }
    UserModel { id: userModel }
    SessionModel { id: sessionModel }

    // --- Background ---
    Rectangle {
        anchors.fill: parent
        color: Maui.Theme.backgroundColor
        z: 0
        Image {
            id: backgroundImage
            anchors.fill: parent
            source: (ColorScheme.backgroundImage && ColorScheme.backgroundImage !== "") ? "file://" + ColorScheme.backgroundImage : ""
            fillMode: Image.PreserveAspectCrop
            visible: false; cache: false
        }
        FastBlur {
            anchors.fill: parent; source: backgroundImage; radius: 64
            visible: backgroundImage.status === Image.Ready; cached: true
        }
        Rectangle {
            anchors.fill: parent; opacity: 0.3
            visible: backgroundImage.status !== Image.Ready
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.lighter(Maui.Theme.backgroundColor, 1.1) }
                GradientStop { position: 1.0; color: Qt.darker(Maui.Theme.backgroundColor, 1.1) }
            }
        }
    }
    Rectangle {
        anchors.fill: parent; color: Maui.Theme.backgroundColor; opacity: 0.76; z: 1
    }

    // --- Top Elements ---
    
    // Top Left: User
    RowLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: Maui.Style.space.big
        spacing: Maui.Style.space.medium
        z: 10

        Label {
            text: qsTr("User")
            color: Maui.Theme.textColor
            font.weight: Font.DemiBold
            verticalAlignment: Text.AlignVCenter
        }
        StyledComboBox {
            id: userCombo
            Layout.preferredWidth: 200
            model: userModel
            textRole: "realName"
        }
    }

    // Top Right: Session
    RowLayout {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Maui.Style.space.big
        spacing: Maui.Style.space.medium
        z: 10

        Label {
            text: qsTr("Session")
            color: Maui.Theme.textColor
            font.weight: Font.DemiBold
            verticalAlignment: Text.AlignVCenter
        }
        StyledComboBox {
            id: sessionCombo
            Layout.preferredWidth: 240
            model: sessionModel
            textRole: "name"
        }
    }

    // Top Center: Clock and Battery
    ColumnLayout {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 80
        spacing: 0
        z: 10

        Label {
            id: timeLabel
            Layout.alignment: Qt.AlignHCenter
            font.pixelSize: 155
            color: Maui.Theme.textColor
            font.bold: true
            
            Timer {
                interval: 1000; running: true; repeat: true
                onTriggered: {
                    var d = new Date()
                    timeLabel.text = Qt.formatDateTime(d, "hh:mm")
                    dateLabel.text = Qt.formatDateTime(d, "dddd, d MMMM yyyy").toLowerCase()
                }
            }
        }

        Label {
            id: dateLabel
            Layout.alignment: Qt.AlignHCenter
            font.pixelSize: 25
            color: Maui.Theme.textColor
            font.weight: Font.Light
        }

        // Spacer between Date and Battery
        Item { height: 16 }

        Label {
            id: batteryLabel
            Layout.alignment: Qt.AlignHCenter
            visible: battery.available
            text: battery.info
            color: Maui.Theme.textColor
            font.weight: Font.Medium
            
            background: Rectangle {
                color: Qt.rgba(0,0,0,0.3)
                radius: 12
            }
            topPadding: 4; bottomPadding: 4; leftPadding: 12; rightPadding: 12
        }
    }

    // --- User and Password ---
    StackLayout {
        id: loginStack
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        currentIndex: 0
        z: 10

        // View 0: Avatar
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.centerIn: parent
                spacing: Maui.Style.space.big

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 150 
                    height: 150
                    radius: 75
                    color: "transparent"
                    border.color: mouseArea.containsMouse ? Maui.Theme.highlightColor : "transparent"
                    border.width: 3
                    Behavior on border.color { ColorAnimation { duration: 150 } }

                    property int uIndex: userCombo.currentIndex
                    property string iconPath: uIndex >= 0 ? userModel.data(userModel.index(uIndex, 0), 259) : ""

                    Maui.Icon {
                        anchors.centerIn: parent
                        width: 128; height: 128
                        source: "user-identity"
                        color: Maui.Theme.textColor
                        visible: avatarImg.status !== Image.Ready
                    }

                    Item {
                        anchors.centerIn: parent
                        width: 138; height: 138
                        Image {
                            id: avatarImg
                            anchors.fill: parent
                            source: parent.parent.iconPath ? "file://" + parent.parent.iconPath : ""
                            fillMode: Image.PreserveAspectCrop
                            visible: false; cache: false
                        }
                        OpacityMask {
                            anchors.fill: avatarImg; source: avatarImg
                            maskSource: Rectangle { width: 138; height: 138; radius: 69 }
                            visible: parent.parent.iconPath && avatarImg.status === Image.Ready
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        enabled: !auth.processing
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            var idx = userCombo.currentIndex
                            if (idx >= 0) {
                                var username = userModel.data(userModel.index(idx, 0), 257)
                                auth.login(username)
                            }
                        }
                    }
                }

                Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: {
                        if (userCombo.currentIndex < 0) return ""
                        return userModel.data(userModel.index(userCombo.currentIndex, 0), 258)
                    }
                    color: Maui.Theme.textColor
                    font.weight: Font.Medium
                }
            }
        }

        // View 1: Password
        Item {
            Layout.fillWidth: true; Layout.fillHeight: true
            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width - Maui.Style.space.big * 2, 400)
                spacing: Maui.Style.space.medium

                Maui.SectionHeader {
                    Layout.fillWidth: true
                    text1: auth.currentPrompt || "Password"
                    text2: "Enter your password to continue"
                }

                Maui.TextField {
                    id: passwordField
                    Layout.fillWidth: true; Layout.preferredHeight: Maui.Style.rowHeight
                    echoMode: auth.isSecret ? TextInput.Password : TextInput.Normal
                    placeholderText: "Enter password"
                    enabled: !auth.processing
                    onAccepted: {
                        if (!auth.processing && text.length > 0) {
                            auth.respond(text)
                        }
                    }

                    palette.highlight: Maui.Theme.highlightColor
                    palette.highlightedText: Maui.Theme.highlightedTextColor

                    background: Rectangle {
                        radius: Maui.Style.radiusV
                        color: {
                            if (passwordField.activeFocus) return ColorScheme.buttonBackground
                            if (passwordFieldHover.containsMouse) return ColorScheme.buttonHover
                            return ColorScheme.viewBackground
                        }
                        border.width: passwordField.activeFocus ? 1 : 1
                        border.color: passwordField.activeFocus ? ColorScheme.buttonFocus : Qt.rgba(1, 1, 1, 0.08)

                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on border.color { ColorAnimation { duration: 150 } }
                        Behavior on border.width { NumberAnimation { duration: 150 } }

                        MouseArea {
                            id: passwordFieldHover
                            anchors.fill: parent
                            hoverEnabled: true
                            propagateComposedEvents: true
                            onPressed: mouse.accepted = false
                        }
                    }

                    Component.onCompleted: {
                        function styleButton(obj) {
                            if (!obj || !obj.children) return
                            for (var i = 0; i < obj.children.length; i++) {
                                var child = obj.children[i]
                                var childStr = child.toString()

                                if (childStr.indexOf("ToolButton") !== -1 ||
                                    childStr.indexOf("Button") !== -1) {

                                    child.flat = true
                                    child.hoverEnabled = false

                                    if (child.background) {
                                        child.background.visible = false
                                    }

                                    child.background = Qt.createQmlObject(
                                        'import QtQuick; Rectangle { color: "transparent"; border.width: 0 }',
                                        child
                                    )
                                }

                                styleButton(child)
                            }
                        }

                        styleButton(passwordField)
                    }

                    SequentialAnimation {
                        id: errorAnimation
                        NumberAnimation { target: passwordField; property: "x"; to: passwordField.x + 10; duration: 50 }
                        NumberAnimation { target: passwordField; property: "x"; to: passwordField.x - 10; duration: 50 }
                        NumberAnimation { target: passwordField; property: "x"; to: passwordField.x; duration: 50 }
                    }
                }

                Label {
                    Layout.fillWidth: true; text: auth.error; color: Maui.Theme.negativeTextColor
                    visible: auth.error !== ""; font.pixelSize: Maui.Style.fontSizes.small; horizontalAlignment: Text.AlignHCenter
                }

                StyledButton {
                    Layout.alignment: Qt.AlignHCenter; text: "Cancel"
                    onClicked: { auth.cancel(); loginStack.currentIndex = 0 }
                }
            }
        }
    }

    // --- Bottom Bar ---
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Maui.Style.space.small 
        width: buttonRow.width + (Maui.Style.space.big * 2)
        height: 60 
        
        color: Maui.Theme.backgroundColor
        radius: 15
        z: 10

        RowLayout {
            id: buttonRow
            anchors.centerIn: parent
            spacing: Maui.Style.space.small
            StyledButton { iconName: "system-suspend"; visible: power.canSuspend(); onClicked: power.suspend() }
            StyledButton { iconName: "system-suspend-hibernate"; visible: power.canHibernate(); onClicked: power.hibernate() }
            StyledButton { iconName: "system-suspend-hibernate"; visible: power.canHybridSleep(); onClicked: power.hybridSleep() }
            StyledButton { iconName: "system-suspend-hibernate"; visible: power.canSuspendThenHibernate(); onClicked: power.suspendThenHibernate() }
            StyledButton { iconName: "system-reboot"; visible: power.canReboot(); onClicked: power.reboot() }
            StyledButton { iconName: "system-shutdown"; visible: power.canPowerOff(); onClicked: power.powerOff() }
        }
    }
}
