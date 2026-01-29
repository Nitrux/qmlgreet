import QtQuick
import QtQuick.Controls
import org.mauikit.controls as Maui
import QmlGreet 1.0

ComboBox {
    id: control

    implicitWidth: 240
    implicitHeight: 36 // Slightly more compact height

    // [MODIFIED] Main Box: Dark, No Border
    background: Rectangle {
        implicitWidth: control.width
        implicitHeight: control.height
        radius: Maui.Style.radiusV
        
        // Matches Cinderward's dark input/header background
        color: ColorScheme.viewBackground 
        
        // [FIX] Removed Cyan border entirely.
        border.color: "transparent" 
    }

    contentItem: Item {
        width: control.width
        height: control.height

        Label {
            anchors.fill: parent
            anchors.leftMargin: 8 // Tighter text alignment
            anchors.rightMargin: 8
            
            text: control.displayText
            font: control.font
            color: ColorScheme.buttonForeground
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
        }
    }

    // [MODIFIED] Popup: Closer, Tighter, Darker
    popup: Popup {
        y: control.height + 2 // [FIX] Closer to the bottom edge
        width: control.width
        implicitHeight: contentItem.implicitHeight
        padding: 2 // [FIX] Reduced outer padding

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: ColorScheme.windowBackground // Deepest dark
            border.color: Qt.rgba(1, 1, 1, 0.08) // Extremely subtle edge, NO Cyan
            border.width: 1
            radius: Maui.Style.radiusV
        }
    }

    // [MODIFIED] List Item: Compact and Dimmer Highlight
    delegate: ItemDelegate {
        width: control.width - 4 // Account for popup padding (2+2)
        height: 32 // [FIX] Compact height
        
        contentItem: Label {
            text: model[control.textRole]
            color: ColorScheme.buttonForeground
            font: control.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            leftPadding: 6 // [FIX] Text closer to edge
        }

        background: Rectangle {
            // [FIX] Dimmer Highlight: Use opacity instead of solid color
            color: ColorScheme.buttonBackground
            opacity: (control.highlightedIndex === index || parent.hovered) ? 0.3 : 0
            radius: Maui.Style.radiusV
            
            Behavior on opacity { NumberAnimation { duration: 50 } }
        }
        
        highlighted: control.highlightedIndex === index
    }
}
