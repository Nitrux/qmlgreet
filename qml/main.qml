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
    color: loginStack.loggingIn ? "black" : "transparent"
    property real contentOpacity: 1

    NumberAnimation {
        id: loginFade
        property string sessionCommand
        target: root
        property: "contentOpacity"
        from: 1
        to: 0
        duration: 250
        easing.type: Easing.InOutQuad
        onFinished: auth.startSession(sessionCommand)
    }

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
            (pad.active ? pinField : passwordField).forceActiveFocus()
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
            (pad.active ? pinField : passwordField).forceActiveFocus()
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
            if (loginStack.attemptUser !== username) loginStack.attempts = 0
            loginStack.attemptUser = username
            loginStack.failure = ""
            loginStack.credentialRejected = false
            auth.login(username)
        }
    }

    function hasAuthenticationPrompt() {
        return auth.currentPrompt.length > 0
                && (auth.requiresInput || !/^(PIN authentication (is not configured|currently unavailable)|Prihlásenie PINom (nie je aktivované|momentálne nedostupné))$/.test(auth.currentPrompt.trim()))
    }

    function cancelLoginPrompt() {
        loginStack.failure = ""
        loginStack.attempts = 0
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
                    (pad.active ? pinField : passwordField).forceActiveFocus()
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
            loginStack.verifying = false
            if (root.hasAuthenticationPrompt()) {
                passwordField.text = ""
                loginStack.currentIndex = 1
                if (auth.requiresInput)
                    Qt.callLater(function() { (pad.active ? pinField : passwordField).forceActiveFocus() })
            }
        }
        onLoginSucceeded: {
            loginStack.verifying = false
            auth.error = ""

            // Ensure valid selection before launch
            if (sessionCombo.currentIndex < 0) {
                selectDefaultSession()
            }

            var idx = sessionCombo.currentIndex
            if (idx >= 0) {
                var cmd = sessionModel.execCommand(idx)
                console.log("Launching session:", cmd)
                loginStack.loggingIn = true
                loginStack.currentIndex = 1
                loginFade.sessionCommand = cmd
                loginFade.start()
            } else {
                auth.error = "No session selected"
            }
        }
        onAuthenticationFailed: {
            loginStack.attempts++
            loginStack.credentialRejected = true
        }
        onErrorChanged: {
            if (auth.error !== "") {
                loginStack.verifying = false
                loginFade.stop()
                root.contentOpacity = 1
                loginStack.loggingIn = false
                loginStack.failure = auth.error
                loginStack.currentIndex = 1
                Qt.callLater(() => retryButton.forceActiveFocus())
            }
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
        opacity: root.contentOpacity
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
        opacity: ConfigOverlayOpacity * root.contentOpacity; visible: ConfigOverlayEnabled; z: 1
    }

    // --- Top Elements ---
    
    // Top Left: User
    RowLayout {
        opacity: root.contentOpacity
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
        opacity: root.contentOpacity
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
        opacity: root.contentOpacity
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
        property int attempts: 0
        property string attemptUser: ""
        property string failure: ""
        property bool credentialRejected: false
        property bool loggingIn: false
        property bool verifying: false
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
                    activeFocusOnTab: true
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
                id: loginStatus
                visible: loginStack.loggingIn || loginStack.verifying
                spacing: Maui.Style.space.small
                width: authenticationView.width
                Maui.SectionHeader {
                    Layout.fillWidth: true
                    text1: loginStack.loggingIn ? qsTr("Logging In") : qsTr("Verifying Credentials")
                    text2: loginStack.loggingIn ? qsTr("Starting your session.") : qsTr("Checking your sign-in details.")
                }
                ProgressBar {
                    Layout.fillWidth: true
                    indeterminate: loginStatus.visible
                }
            }

            ColumnLayout {
                id: authenticationView
                anchors.centerIn: parent
                width: Math.min(parent.width - Maui.Style.space.big * 2, 400)
                visible: !loginStatus.visible && (hasPrompt || loginStack.failure.length > 0)
                readonly property bool hasPrompt: !loginStack.failure && root.hasAuthenticationPrompt()
                spacing: Maui.Style.space.medium

                readonly property string step: !visible || !auth.currentPrompt ? ""
                    : !auth.requiresInput ? "instruction" : pad.active ? "pin" : "password"
                onStepChanged: if (step) stepFade.restart()

                NumberAnimation {
                    id: stepFade
                    target: authenticationView
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: Maui.Style.units.longDuration
                    easing.type: Easing.OutCubic
                }

                Maui.SectionHeader {
                    id: promptHeader
                    visible: authenticationView.hasPrompt
                    Layout.fillWidth: true
                    readonly property bool scanInstruction: !auth.requiresInput
                        && /^(please\s+)?(place|swipe|press|touch|scan)\b/i.test(auth.currentPrompt.trim())
                    readonly property bool fingerprintMismatch: !auth.requiresInput
                        && auth.currentPrompt.trim() === "Failed to match fingerprint"
                    text1: fingerprintMismatch ? qsTr("Fingerprint did not match.")
                        : scanInstruction ? qsTr("Place your finger on the sensor.")
                        : auth.currentPrompt
                    text2: auth.requiresInput
                        ? (pad.active ? qsTr("Enter your PIN") : qsTr("Enter your password"))
                        : (scanInstruction || fingerprintMismatch) ? "" : qsTr("Follow the authentication instructions")
                }

                Maui.PasswordField {
                    id: passwordField
                    Layout.fillWidth: true
                    Layout.preferredHeight: Maui.Style.rowHeight
                    visible: authenticationView.hasPrompt && auth.requiresInput && !pad.active
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
                        if (!auth.processing && auth.requiresInput) {
                            loginStack.verifying = true
                            auth.respond(text)
                            clear()
                        }
                    }


                }

                TextField {
                    id: pinField
                    Layout.fillWidth: true
                    Layout.preferredHeight: Maui.Style.rowHeight
                    visible: pad.active
                    onVisibleChanged: if (visible) forceActiveFocus()
                    horizontalAlignment: TextInput.AlignHCenter
                    echoMode: TextInput.Password
                    passwordMaskDelay: 0
                    passwordCharacter: "●"
                    inputMethodHints: Qt.ImhDigitsOnly | Qt.ImhHiddenText | Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
                    validator: RegularExpressionValidator { regularExpression: /[0-9]+/ }
                    readonly property bool canSubmit: visible && acceptableInput && auth.requiresInput && auth.isSecret && !auth.processing
                    onAccepted: submitPin()
                    function submitPin() {
                        if (!canSubmit) return
                        const answer = text
                        clear()
                        loginStack.verifying = true
                        auth.respond(answer)
                    }
                }

                GridLayout {
                    id: pad
                    readonly property bool active: authenticationView.hasPrompt && auth.requiresInput && auth.isSecret
                        && /\bpin\b/i.test(auth.currentPrompt)
                    visible: active
                    Layout.alignment: Qt.AlignHCenter
                    columns: 3
                    rowSpacing: Maui.Style.space.big
                    columnSpacing: rowSpacing
                    onVisibleChanged: if (!visible) pinField.clear()

                    Repeater {
                        model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "⌫", "0", "✓"]
                        Button {
                            required property string modelData
                            Layout.preferredWidth: Maui.Style.rowHeight * 1.5
                            Layout.preferredHeight: Maui.Style.rowHeight * 1.5
                            icon.name: modelData === "⌫" ? "edit-clear" : ""
                            text: modelData === "⌫" ? "" : modelData
                            Accessible.name: modelData === "✓" ? qsTr("Submit PIN")
                                : modelData === "⌫" ? qsTr("Delete digit") : modelData
                            enabled: modelData !== "✓" || pinField.canSubmit
                            onClicked: {
                                if (modelData === "✓") {
                                    pinField.submitPin()
                                } else if (modelData === "⌫") {
                                    pinField.text = pinField.text.slice(0, -1)
                                } else {
                                    pinField.text += modelData
                                }
                            }
                        }
                    }
                }

                Maui.Chip {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Maui.Style.space.small
                    visible: promptHeader.visible && promptHeader.scanInstruction
                    enabled: false
                    hoverEnabled: false
                    Maui.Theme.colorSet: Maui.Theme.Window
                    color: Maui.Theme.highlightColor
                    spacing: 0

                    SequentialAnimation on opacity {
                        running: promptHeader.visible && promptHeader.scanInstruction
                        loops: Animation.Infinite
                        NumberAnimation {
                            from: 0.35
                            to: 1
                            duration: 450
                        }
                        NumberAnimation {
                            from: 1
                            to: 0.35
                            duration: 450
                        }
                    }
                }

                Maui.SectionHeader {
                    Layout.fillWidth: true
                    visible: loginStack.failure.length > 0
                    text1: loginStack.credentialRejected
                        ? qsTr("Authentication Failed (%1)").arg(loginStack.attempts) : qsTr("Login Failed")
                    text2: loginStack.credentialRejected
                        ? qsTr("Try again or cancel to return to the login screen.") : loginStack.failure
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: Maui.Style.space.small
                    Button {
                        id: retryButton
                        Layout.preferredWidth: Math.max(implicitWidth, cancelButton.implicitWidth)
                        visible: loginStack.failure.length > 0
                        enabled: !auth.processing
                        text: qsTr("Try Again")
                        onClicked: root.startSelectedUserLogin()
                    }
                    Button {
                        id: cancelButton
                        Layout.preferredWidth: retryButton.visible ? Math.max(implicitWidth, retryButton.implicitWidth) : implicitWidth
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
    }

    // --- Bottom Bar ---
    Rectangle {
        opacity: root.contentOpacity
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
