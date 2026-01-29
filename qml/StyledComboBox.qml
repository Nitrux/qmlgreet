import QtQuick
import QtQuick.Controls
import org.mauikit.controls as Maui
import QmlGreet 1.0

ComboBox {
    id: control

    implicitWidth: 200
    implicitHeight: 40

    background: Rectangle {
        implicitWidth: control.width
        implicitHeight: control.height
        radius: Maui.Style.radiusV
        
        color: {
            if (control.pressed) return ColorScheme.buttonFocus
            if (control.hovered) return ColorScheme.buttonHover
            return ColorScheme.buttonBackground
        }

        border.width: (control.hovered || control.pressed) ? 1 : 0
        border.color: ColorScheme.buttonFocus

        Behavior on color { ColorAnimation { duration: 100 } }
    }

    contentItem: Item {
        width: control.width
        height: control.height

        Label {
            anchors.fill: parent
            anchors.leftMargin: Maui.Style.space.medium
            anchors.rightMargin: Maui.Style.space.medium
            
            text: control.displayText
            font: control.font
            color: ColorScheme.buttonForeground
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }
    }

    popup: Popup {
        y: control.height + 4 // [MODIFIED] Slight offset for "floating" look
        width: control.width
        implicitHeight: contentItem.implicitHeight
        padding: 4 // [MODIFIED] Added padding around the list

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: ColorScheme.buttonBackground
            border.color: ColorScheme.buttonFocus
            border.width: 1
            radius: Maui.Style.radiusV
        }
    }

    // [NEW] Custom Delegate to match Cinderward dropdown style
    delegate: ItemDelegate {
        width: control.width - 8 // Account for popup padding
        height: 36 // Fixed comfortable height
        
        contentItem: Label {
            text: model[control.textRole]
            color: ColorScheme.buttonForeground
            font: control.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            color: parent.hovered ? ColorScheme.buttonHover : "transparent"
            radius: Maui.Style.radiusV
        }
        
        highlighted: control.highlightedIndex === index
    }
}
