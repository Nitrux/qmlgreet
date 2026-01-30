import QtQuick
import QtQuick.Controls
import QtQml.Models 2.15
import org.mauikit.controls as Maui
import QmlGreet 1.0

ComboBox {
    id: control

    implicitWidth: 240
    implicitHeight: 36

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

    popup: Popup {
        id: pop
        y: control.height + 4
        width: control.width
        padding: 4

        height: Math.min(control.count * 32, 192) + (padding * 2)

        contentItem: ListView {
            id: listView
            clip: true

            model: DelegateModel {
                model: control.model
                
                delegate: ItemDelegate {
                    width: control.width - 8
                    height: 32
                    padding: 0
                    
                    // Manual Highlight Check
                    highlighted: control.highlightedIndex === index

                    contentItem: Label {
                        // Safe text binding via the DelegateModel wrapper
                        text: model[control.textRole]
                        
                        color: ColorScheme.buttonForeground
                        font: control.font
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 8
                    }

                    background: Rectangle {
                        anchors.fill: parent
                        anchors.margins: 2 
                        radius: Maui.Style.radiusV
                        
                        color: ColorScheme.buttonBackground
                        opacity: (parent.highlighted || parent.hovered) ? 0.3 : 0.0
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: control.highlightedIndex = index
                        onClicked: {
                            control.currentIndex = index
                            control.popup.close()
                        }
                    }
                }
            }
            
            currentIndex: control.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds
            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: ColorScheme.windowBackground
            border.color: Qt.rgba(1, 1, 1, 0.08)
            border.width: 1
            radius: Maui.Style.radiusV
        }
    }
}
