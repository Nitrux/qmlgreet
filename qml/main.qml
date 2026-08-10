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

    function visiblePowerButtons() {
        var buttons = [suspendButton, hibernateButton, hybridSleepButton, suspendThenHibernateButton, rebootButton, shutdownButton]
        var visibleButtons = []

        for (var i = 0; i < buttons.length; i++) {
            if (buttons[i] && buttons[i].visible) {
                visibleButtons.push(buttons[i])
            }
        }

        return visibleButtons
    }

    function firstVisiblePowerButton(fallbackItem) {
        var buttons = visiblePowerButtons()
        return buttons.length > 0 ? buttons[0] : fallbackItem
    }

    function lastVisiblePowerButton(fallbackItem) {
        var buttons = visiblePowerButtons()
        return buttons.length > 0 ? buttons[buttons.length - 1] : fallbackItem
    }

    function focusLoginSelection() {
        if (loginStack.currentIndex === 1) {
            passwordField.forceActiveFocus()
            return
        }

        if (userModel.rowCount() > 0) {
            avatarButton.forceActiveFocus()
        } else {
            sessionCombo.forceActiveFocus()
        }
    }

    function focusInitialControl() {
        if (loginStack.currentIndex === 1) {
            passwordField.forceActiveFocus()
            return
        }

        if (userModel.rowCount() > 0) {
            userCombo.forceActiveFocus()
        } else {
            sessionCombo.forceActiveFocus()
        }
    }

    function startSelectedUserLogin() {
        if (auth.processing) {
            return
        }

        var idx = userCombo.currentIndex
        if (idx >= 0) {
            var username = userModel.data(userModel.index(idx, 0), 257)
            auth.login(username)
        }
    }

    function cancelLoginPrompt() {
        auth.cancel()
        loginStack.currentIndex = 0
    }

    function movePowerFocus(currentButton, step, allowExit) {
        var buttons = visiblePowerButtons()

        if (buttons.length === 0) {
            focusLoginSelection()
            return
        }

        var index = buttons.indexOf(currentButton)
        if (index === -1) {
            buttons[0].forceActiveFocus()
            return
        }

        var nextIndex = index + step
        if (nextIndex < 0 || nextIndex >= buttons.length) {
            if (allowExit) {
                if (loginStack.currentIndex === 1) {
                    passwordField.forceActiveFocus()
                } else if (step > 0) {
                    userCombo.forceActiveFocus()
                } else {
                    avatarButton.forceActiveFocus()
                }
            } else {
                buttons[index].forceActiveFocus()
            }
            return
        }

        buttons[nextIndex].forceActiveFocus()
    }

    Component.onCompleted: {
        layerShell.activate()
        root.visible = true
        if (userModel.rowCount() > 0) userCombo.currentIndex = 0

        selectDefaultSession()
        Qt.callLater(function() { focusInitialControl() })
    }

    Connections {
        target: sessionModel
        function onRowsInserted() { selectDefaultSession() }
        function onModelReset() { selectDefaultSession() }
    }

    AuthWrapper {
        id: auth
        onPromptChanged: {
            if (auth.currentPrompt !== "") {
                // Only switch to password view if there's actually a prompt
                passwordField.text = ""
                loginStack.currentIndex = 1
            }
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
        onErrorChanged: {
            if (auth.error !== "") {
                errorAnimation.start()
                // Show error message for 2 seconds before resetting to avatar view
                errorResetTimer.start()
            }
        }
    }

    // Timer to reset view after showing error message
    Timer {
        id: errorResetTimer
        interval: 2000
        repeat: false
        onTriggered: {
            loginStack.currentIndex = 0
        }
    }

    SystemPower { id: power }
    SystemBattery {
        id: battery
        debugBattery: ConfigDebugBattery
    }
    SessionModel { id: sessionModel }

    // --- Background ---
    Rectangle {
        anchors.fill: parent
        color: Maui.Theme.backgroundColor
        z: 0
        Image {
            id: backgroundImage
            anchors.fill: parent
            source: (ConfigBackgroundImage && ConfigBackgroundImage !== "") ? "file://" + ConfigBackgroundImage : ""
            fillMode: Image.PreserveAspectCrop
            visible: false; cache: false
        }
        FastBlur {
            anchors.fill: parent; source: backgroundImage
            radius: ConfigBlurEnabled ? 64 : 0
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
        anchors.fill: parent; color: Maui.Theme.backgroundColor
        opacity: ConfigOverlayOpacity; visible: ConfigOverlayEnabled; z: 1
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
        ComboBox {
            id: userCombo
            Layout.preferredWidth: 200
            model: userModel
            textRole: "realName"
            KeyNavigation.tab: sessionCombo
            KeyNavigation.backtab: root.lastVisiblePowerButton(avatarButton)
            Keys.onRightPressed: function(event) {
                if (!popup.visible) {
                    sessionCombo.forceActiveFocus()
                    event.accepted = true
                }
            }
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
        ComboBox {
            id: sessionCombo
            Layout.preferredWidth: 240
            model: sessionModel
            textRole: "name"
            KeyNavigation.tab: avatarButton
            KeyNavigation.backtab: userCombo
            Keys.onLeftPressed: function(event) {
                if (!popup.visible) {
                    userCombo.forceActiveFocus()
                    event.accepted = true
                }
            }
        }
    }

    // Top Center: Clock and Battery
    ColumnLayout {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: Math.max(48, parent.height * 0.08)
        width: Math.min(900, parent.width - Maui.Style.space.big * 2)
        spacing: 12
        z: 10

        Maui.IconLabel {
            id: timeLabel
            Layout.fillWidth: true
            Layout.preferredHeight: font.pixelSize * 1.05
            Layout.alignment: Qt.AlignHCenter
            display: ToolButton.TextOnly
            spacing: 0
            color: Maui.Theme.textColor
            alignment: Qt.AlignHCenter
            font.pixelSize: 155
            font.weight: Font.Bold

            Timer {
                interval: 1000; running: true; repeat: true
                onTriggered: {
                    var d = new Date()
                    timeLabel.text = Qt.formatDateTime(d, "hh:mm")
                    var formattedDate = Qt.formatDateTime(d, "dddd, d MMMM yyyy")
                    dateLabel.text = ConfigLowercaseDate ? formattedDate.toLowerCase() : formattedDate
                }
            }
        }

        Maui.IconLabel {
            id: dateLabel
            Layout.fillWidth: true
            Layout.preferredHeight: font.pixelSize * 1.3
            Layout.alignment: Qt.AlignHCenter
            display: ToolButton.TextOnly
            spacing: 0
            color: Maui.Theme.textColor
            alignment: Qt.AlignHCenter
            font.pixelSize: 25
            font.weight: Font.Light
        }

        // Spacer between Date and Battery
        Item {
            Layout.preferredHeight: 16
        }

        Maui.Chip {
            id: batteryLabel
            Layout.alignment: Qt.AlignHCenter
            visible: battery.available
            enabled: false
            hoverEnabled: false
            color: Qt.rgba(0, 0, 0, 0.3)
            label.font.weight: Font.Medium
            implicitWidth: batteryRow.implicitWidth + Maui.Style.space.medium * 2
            implicitHeight: batteryRow.implicitHeight + Maui.Style.space.small * 2

            contentItem: RowLayout {
                id: batteryRow
                spacing: Maui.Style.space.small

                Maui.Icon {
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                    source: battery.iconName
                    visible: IconMode !== "nerd"
                }

                Label {
                    Layout.preferredWidth: 16
                    Layout.preferredHeight: 16
                    text: "\uf240"
                    font.family: "Symbols Nerd Font"
                    textFormat: Text.PlainText
                    renderType: Text.QtRendering
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    visible: IconMode === "nerd"
                }

                Maui.IconLabel {
                    display: ToolButton.TextOnly
                    text: battery.info
                    font.weight: Font.Medium
                }
            }
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

        onCurrentIndexChanged: {
            Qt.callLater(function() { focusLoginSelection() })
        }

        // View 0: Avatar
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.centerIn: parent
                spacing: Maui.Style.space.big

                Rectangle {
                    id: avatarButton
                    Layout.alignment: Qt.AlignHCenter
                    width: 150 
                    height: 150
                    radius: 75
                    color: avatarButton.activeFocus ? Qt.rgba(1, 1, 1, 0.04) : "transparent"
                    border.color: (avatarButton.activeFocus || mouseArea.containsMouse) ? Maui.Theme.highlightColor : "transparent"
                    border.width: 3
                    activeFocusOnTab: loginStack.currentIndex === 0 && mouseArea.enabled
                    KeyNavigation.tab: root.firstVisiblePowerButton(userCombo)
                    KeyNavigation.backtab: sessionCombo
                    Behavior on border.color { ColorAnimation { duration: 150 } }
                    Behavior on color { ColorAnimation { duration: 150 } }

                    property int uIndex: userCombo.currentIndex
                    property string iconPath: uIndex >= 0 ? userModel.data(userModel.index(uIndex, 0), 259) : ""

                    Keys.onPressed: function(event) {
                        switch (event.key) {
                        case Qt.Key_Return:
                        case Qt.Key_Enter:
                        case Qt.Key_Space:
                            root.startSelectedUserLogin()
                            event.accepted = true
                            break
                        case Qt.Key_Left:
                            userCombo.forceActiveFocus()
                            event.accepted = true
                            break
                        case Qt.Key_Right:
                            sessionCombo.forceActiveFocus()
                            event.accepted = true
                            break
                        case Qt.Key_Up:
                            userCombo.forceActiveFocus()
                            event.accepted = true
                            break
                        case Qt.Key_Down:
                            root.firstVisiblePowerButton(avatarButton).forceActiveFocus()
                            event.accepted = true
                            break
                        }
                    }

                    Item {
                        anchors.centerIn: parent
                        width: 138; height: 138

                        Image {
                            id: avatarImg
                            anchors.fill: parent
                            source: {
                                if (!ConfigShowAvatars) {
                                    return "qrc:/icons/user-avatar.svg"
                                }

                                var iconPath = parent.parent.iconPath
                                console.log("Avatar iconPath:", iconPath)
                                if (!iconPath) {
                                    console.log("No iconPath, using fallback")
                                    return "qrc:/icons/user-avatar.svg"
                                }
                                if (iconPath.startsWith("qrc:")) {
                                    console.log("Using QRC path:", iconPath)
                                    return iconPath
                                }
                                console.log("Using file path:", iconPath)
                                return "file://" + iconPath
                            }
                            fillMode: Image.PreserveAspectCrop
                            visible: false
                            cache: false
                            // Use fallback if image fails to load
                            onStatusChanged: {
                                console.log("Avatar status:", status, "source:", source)
                                if (status === Image.Error) {
                                    console.log("Image failed to load, trying fallback")
                                    source = "qrc:/icons/user-avatar.svg"
                                }
                            }
                        }

                        OpacityMask {
                            anchors.fill: avatarImg
                            source: avatarImg
                            maskSource: Rectangle { width: 138; height: 138; radius: 69 }
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        enabled: !auth.processing
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onPressed: avatarButton.forceActiveFocus()
                        onClicked: root.startSelectedUserLogin()
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

                Maui.PasswordField {
                    id: passwordField
                    Layout.fillWidth: true
                    Layout.preferredHeight: Maui.Style.rowHeight
                    enabled: !auth.processing
                    readOnly: auth.processing
                    echoMode: auth.isSecret ? TextInput.Password : TextInput.Normal
                    passwordMaskDelay: 0
                    actions: []
                    icon.source: ""
                    inputMethodHints: Qt.ImhHiddenText
                        | Qt.ImhSensitiveData
                        | Qt.ImhNoPredictiveText
                        | Qt.ImhNoAutoUppercase
                    placeholderText: auth.processing ? "" : qsTr("Enter password")
                    passwordCharacter: "●"
                    selectByMouse: false
                    KeyNavigation.tab: cancelButton
                    KeyNavigation.backtab: cancelButton
                    Maui.Controls.status: auth.error !== ""
                        ? Maui.Controls.Negative : 0
                    Keys.onEscapePressed: function(event) {
                        root.cancelLoginPrompt()
                        event.accepted = true
                    }
                    Keys.onDownPressed: function(event) {
                        cancelButton.forceActiveFocus()
                        event.accepted = true
                    }
                    onAccepted: {
                        if (!auth.processing && text.length > 0) {
                            auth.respond(text)
                        }
                    }

                    SequentialAnimation {
                        id: errorAnimation
                        NumberAnimation { target: passwordField; property: "x"; to: passwordField.x + 10; duration: 50 }
                        NumberAnimation { target: passwordField; property: "x"; to: passwordField.x - 10; duration: 50 }
                        NumberAnimation { target: passwordField; property: "x"; to: passwordField.x; duration: 50 }
                    }
                }

                Maui.IconLabel {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.maximumWidth: parent.width
                    display: ToolButton.TextOnly
                    spacing: 0
                    visible: auth.error !== ""
                    text: auth.error
                    color: Maui.Theme.negativeTextColor
                    alignment: Text.AlignHCenter
                    label.wrapMode: Text.Wrap
                }

                Button {
                    id: cancelButton
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: Maui.Style.rowHeight
                    enabled: !auth.processing
                    activeFocusOnTab: true
                    text: qsTr("Cancel")
                    KeyNavigation.up: passwordField
                    KeyNavigation.tab: passwordField
                    KeyNavigation.backtab: passwordField
                    Keys.onEscapePressed: function(event) {
                        root.cancelLoginPrompt()
                        event.accepted = true
                    }
                    onClicked: root.cancelLoginPrompt()
                }
            }
        }
    }

    // --- Bottom Bar ---
    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Maui.Style.space.medium
        width: buttonRow.width + (Maui.Style.space.medium * 2)
        height: buttonRow.height + (Maui.Style.space.medium * 2)
        color: Qt.alpha(Maui.Theme.backgroundColor, 0.88)
        radius: Maui.Style.radiusV + 6
        border.color: Qt.alpha(Maui.Theme.textColor, 0.14)
        border.width: 1
        z: 10

        RowLayout {
            id: buttonRow
            anchors.centerIn: parent
            spacing: Maui.Style.space.small

            Button {
                id: suspendButton
                implicitWidth: 48
                implicitHeight: 48
                icon.width: 40
                icon.height: 40
                padding: 0
                hoverEnabled: true
                scale: hovered ? 1.12 : 1.0
                Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                background: Rectangle {
                    radius: Maui.Style.radiusV
                    color: suspendButton.activeFocus ? Qt.alpha(Maui.Theme.highlightColor, 0.18) : suspendButton.hovered ? Qt.alpha(Maui.Theme.textColor, 0.08) : "transparent"
                }
                icon.name: "system-suspend"
                display: AbstractButton.IconOnly
                visible: power.canSuspend()
                Keys.onLeftPressed: root.movePowerFocus(suspendButton, -1, false)
                Keys.onRightPressed: root.movePowerFocus(suspendButton, 1, false)
                Keys.onUpPressed: root.focusLoginSelection()
                onClicked: power.suspend()
            }

            Button {
                id: hibernateButton
                implicitWidth: 48
                implicitHeight: 48
                icon.width: 40
                icon.height: 40
                padding: 0
                hoverEnabled: true
                scale: hovered ? 1.12 : 1.0
                Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                background: Rectangle {
                    radius: Maui.Style.radiusV
                    color: hibernateButton.activeFocus ? Qt.alpha(Maui.Theme.highlightColor, 0.18) : hibernateButton.hovered ? Qt.alpha(Maui.Theme.textColor, 0.08) : "transparent"
                }
                icon.name: "system-suspend-hibernate"
                display: AbstractButton.IconOnly
                visible: power.canHibernate()
                Keys.onLeftPressed: root.movePowerFocus(hibernateButton, -1, false)
                Keys.onRightPressed: root.movePowerFocus(hibernateButton, 1, false)
                Keys.onUpPressed: root.focusLoginSelection()
                onClicked: power.hibernate()
            }

            Button {
                id: hybridSleepButton
                implicitWidth: 48
                implicitHeight: 48
                icon.width: 40
                icon.height: 40
                padding: 0
                hoverEnabled: true
                scale: hovered ? 1.12 : 1.0
                Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                background: Rectangle {
                    radius: Maui.Style.radiusV
                    color: hybridSleepButton.activeFocus ? Qt.alpha(Maui.Theme.highlightColor, 0.18) : hybridSleepButton.hovered ? Qt.alpha(Maui.Theme.textColor, 0.08) : "transparent"
                }
                icon.name: "system-suspend-hibernate"
                display: AbstractButton.IconOnly
                visible: power.canHybridSleep()
                Keys.onLeftPressed: root.movePowerFocus(hybridSleepButton, -1, false)
                Keys.onRightPressed: root.movePowerFocus(hybridSleepButton, 1, false)
                Keys.onUpPressed: root.focusLoginSelection()
                onClicked: power.hybridSleep()
            }

            Button {
                id: suspendThenHibernateButton
                implicitWidth: 48
                implicitHeight: 48
                icon.width: 40
                icon.height: 40
                padding: 0
                hoverEnabled: true
                scale: hovered ? 1.12 : 1.0
                Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                background: Rectangle {
                    radius: Maui.Style.radiusV
                    color: suspendThenHibernateButton.activeFocus ? Qt.alpha(Maui.Theme.highlightColor, 0.18) : suspendThenHibernateButton.hovered ? Qt.alpha(Maui.Theme.textColor, 0.08) : "transparent"
                }
                icon.name: "system-suspend-hibernate"
                display: AbstractButton.IconOnly
                visible: power.canSuspendThenHibernate()
                Keys.onLeftPressed: root.movePowerFocus(suspendThenHibernateButton, -1, false)
                Keys.onRightPressed: root.movePowerFocus(suspendThenHibernateButton, 1, false)
                Keys.onUpPressed: root.focusLoginSelection()
                onClicked: power.suspendThenHibernate()
            }

            Button {
                id: rebootButton
                implicitWidth: 48
                implicitHeight: 48
                icon.width: 40
                icon.height: 40
                padding: 0
                hoverEnabled: true
                scale: hovered ? 1.12 : 1.0
                Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                background: Rectangle {
                    radius: Maui.Style.radiusV
                    color: rebootButton.activeFocus ? Qt.alpha(Maui.Theme.highlightColor, 0.18) : rebootButton.hovered ? Qt.alpha(Maui.Theme.textColor, 0.08) : "transparent"
                }
                icon.name: "system-reboot"
                display: AbstractButton.IconOnly
                visible: power.canReboot()
                Keys.onLeftPressed: root.movePowerFocus(rebootButton, -1, false)
                Keys.onRightPressed: root.movePowerFocus(rebootButton, 1, false)
                Keys.onUpPressed: root.focusLoginSelection()
                onClicked: power.reboot()
            }

            Button {
                id: shutdownButton
                implicitWidth: 48
                implicitHeight: 48
                icon.width: 40
                icon.height: 40
                padding: 0
                hoverEnabled: true
                scale: hovered ? 1.12 : 1.0
                Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                background: Rectangle {
                    radius: Maui.Style.radiusV
                    color: shutdownButton.activeFocus ? Qt.alpha(Maui.Theme.highlightColor, 0.18) : shutdownButton.hovered ? Qt.alpha(Maui.Theme.textColor, 0.08) : "transparent"
                }
                icon.name: "system-shutdown"
                display: AbstractButton.IconOnly
                visible: power.canPowerOff()
                Keys.onLeftPressed: root.movePowerFocus(shutdownButton, -1, false)
                Keys.onRightPressed: root.movePowerFocus(shutdownButton, 1, false)
                Keys.onUpPressed: root.focusLoginSelection()
                onClicked: power.powerOff()
            }
        }
    }
}
