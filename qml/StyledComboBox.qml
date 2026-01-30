import QtQuick
import QtQuick.Controls
import org.mauikit.controls as Maui
import QmlGreet 1.0

ComboBox {
    id: control

    implicitWidth: 240
    implicitHeight: 36

    // 1. Closed Box
    background: Rectangle {
        radius: Maui.Style.radiusV
        color: ColorScheme.viewBackground
        border.color: "transparent"
    }

    contentItem: Label {
        leftPadding: 8
        rightPadding: 8
        text: control.displayText
        font: control.font
        color: ColorScheme.buttonForeground
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignLeft
        elide: Text.ElideRight
    }

    // 2. Popup
    popup: Popup {
        y: control.height + 4
        width: control.width
        padding: 4 // Outer padding for the "floating" look

        // Simple Math: 32px per item. Cap at 192px (6 items).
        height: Math.min(control.count * 32, 192) + (padding * 2)

        contentItem: ListView {
            clip: true
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: ColorScheme.windowBackground
            border.color: Qt.rgba(1, 1, 1, 0.08)
            border.width: 1
            radius: Maui.Style.radiusV
        }
    }

    // 3. List Item
    delegate: ItemDelegate {
        width: control.width - 8 // Account for popup padding
        height: 32
        padding: 0
        
        contentItem: Label {
            // [FIX] If this row is the selected one, use the known-good displayText.
            // Otherwise, look it up in the model as usual.
            text: (index === control.currentIndex) ? control.displayText : model[control.textRole]
            
            color: ColorScheme.buttonForeground
            font: control.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            leftPadding: 8
        }

        background: Rectangle {
            // [FIX] Add margins so the highlight "floats" inside the row
            anchors.fill: parent
            anchors.margins: 2 
            radius: Maui.Style.radiusV
            
            color: ColorScheme.buttonBackground
            opacity: (parent.highlighted || parent.hovered) ? 0.3 : 0.0
        }
        
        highlighted: control.highlightedIndex === index
    }
}
