import QtQuick
import QtQuick.Controls
import org.mauikit.controls as Maui
import QmlGreet 1.0

Rectangle {
    id: root

    property string iconName: ""
    signal clicked()

    implicitWidth: 40
    implicitHeight: 40

    color: {
        if (!mouseArea.enabled) return ColorScheme.buttonBackground.darker(1.2)
        if (mouseArea.pressed) return ColorScheme.buttonFocus
        if (mouseArea.containsMouse) return ColorScheme.buttonHover
        return ColorScheme.buttonBackground
    }

    radius: Maui.Style.radiusV
    border.width: mouseArea.containsMouse || mouseArea.pressed ? 1 : 0
    border.color: ColorScheme.buttonFocus

    Behavior on color {
        ColorAnimation { duration: 100 }
    }

    Behavior on border.width {
        NumberAnimation { duration: 100 }
    }

    Maui.Icon {
        anchors.centerIn: parent
        source: root.iconName
        width: 22
        height: 22
        color: ColorScheme.buttonForeground
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
