import QtQuick
import QtQuick.Controls
import org.mauikit.controls as Maui
import QmlGreet 1.0

Rectangle {
    id: root

    property string iconName: ""
    property string text: ""
    signal clicked()

    implicitWidth: text !== "" ? (label.implicitWidth + Maui.Style.space.big * 2) : 48 // Wider click area
    implicitHeight: 40

    color: {
        if (!mouseArea.enabled) return "transparent"
        if (mouseArea.pressed) return ColorScheme.buttonFocus
        if (mouseArea.containsMouse) return ColorScheme.buttonHover
        return text !== "" ? ColorScheme.buttonBackground : "transparent"
    }

    radius: Maui.Style.radiusV
    border.width: (mouseArea.containsMouse || mouseArea.pressed) ? 1 : 0
    border.color: ColorScheme.buttonFocus

    Behavior on color { ColorAnimation { duration: 100 } }
    Behavior on border.width { NumberAnimation { duration: 100 } }

    Maui.Icon {
        anchors.centerIn: parent
        source: root.iconName
        width: 32
        height: 32
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
        onClicked: root.clicked()
    }
}
