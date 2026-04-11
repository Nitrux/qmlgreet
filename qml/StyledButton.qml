import QtQuick
import QtQuick.Controls
import org.mauikit.controls as Maui
import QmlGreet 1.0

Rectangle {
    id: root

    property string iconName: ""
    property string text: ""
    property alias enabled: mouseArea.enabled
    property var keyNavLeft
    property var keyNavRight
    property var keyNavUp
    property var keyNavDown
    property var keyNavTab
    property var keyNavBacktab
    property var keyNavEscape
    signal clicked()

    implicitWidth: text !== "" ? (label.implicitWidth + Maui.Style.space.big * 2) : 48
    implicitHeight: 40
    activeFocusOnTab: visible && enabled
    focus: false

    color: {
        if (!root.enabled) return "transparent"
        if (mouseArea.pressed) return ColorScheme.buttonFocus
        if (root.activeFocus) return text !== "" ? ColorScheme.buttonBackground : Qt.rgba(1, 1, 1, 0.08)
        if (mouseArea.containsMouse) return ColorScheme.buttonHover
        return text !== "" ? ColorScheme.buttonBackground : "transparent"
    }

    radius: Maui.Style.radiusV
    border.width: (mouseArea.containsMouse || mouseArea.pressed || root.activeFocus) ? 1 : 0
    border.color: ColorScheme.buttonFocus

    Behavior on color { ColorAnimation { duration: 100 } }
    Behavior on border.width { NumberAnimation { duration: 100 } }

    Keys.onPressed: function(event) {
        if (!root.enabled) {
            return
        }

        switch (event.key) {
        case Qt.Key_Return:
        case Qt.Key_Enter:
        case Qt.Key_Space:
            root.clicked()
            event.accepted = true
            break
        case Qt.Key_Tab:
            if (event.modifiers & Qt.ShiftModifier) {
                if (root.keyNavBacktab) {
                    root.keyNavBacktab()
                    event.accepted = true
                }
            } else if (root.keyNavTab) {
                root.keyNavTab()
                event.accepted = true
            }
            break
        case Qt.Key_Backtab:
            if (root.keyNavBacktab) {
                root.keyNavBacktab()
                event.accepted = true
            }
            break
        case Qt.Key_Left:
            if (root.keyNavLeft) {
                root.keyNavLeft()
                event.accepted = true
            }
            break
        case Qt.Key_Right:
            if (root.keyNavRight) {
                root.keyNavRight()
                event.accepted = true
            }
            break
        case Qt.Key_Up:
            if (root.keyNavUp) {
                root.keyNavUp()
                event.accepted = true
            }
            break
        case Qt.Key_Down:
            if (root.keyNavDown) {
                root.keyNavDown()
                event.accepted = true
            }
            break
        case Qt.Key_Escape:
            if (root.keyNavEscape) {
                root.keyNavEscape()
                event.accepted = true
            }
            break
        }
    }

    Maui.Icon {
        anchors.centerIn: parent
        source: root.iconName
        width: 40
        height: 40
        color: ColorScheme.buttonForeground
        visible: root.text === "" && root.iconName !== ""
    }

    Label {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: ColorScheme.buttonForeground
        font.weight: Font.Medium
        visible: root.text !== ""
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onPressed: root.forceActiveFocus()
        onClicked: root.clicked()
    }
}
