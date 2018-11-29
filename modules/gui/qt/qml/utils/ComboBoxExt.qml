import QtQuick 2.10
import QtQuick.Controls 2.3

import "qrc:///style/"

ComboBox {
    id: control

    font.pixelSize: VLCStyle.fontSize_normal

    delegate: ItemDelegate {
        width: control.width
        leftPadding: control.leftPadding
        background: Item {}
        contentItem: Text {
            text: control.textRole ? (Array.isArray(control.model) ? modelData[control.textRole] : model[control.textRole]) : modelData
            color: VLCStyle.colors.buttonText
            font: control.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        highlighted: control.highlightedIndex === index
    }

    indicator: Canvas {
        id: canvas
        x: control.width - width - control.rightPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: 12
        height: 8
        contextType: "2d"

        Connections {
            target: control
            onPressedChanged: canvas.requestPaint()
        }

        onPaint: {
            context.reset();
            context.moveTo(0, 0);
            context.lineTo(width, 0);
            context.lineTo(width / 2, height);
            context.closePath();
            context.fillStyle = control.activeFocus ? VLCStyle.colors.accent : VLCStyle.colors.buttonText;
            context.fill();
        }
    }

    contentItem: Text {
        leftPadding: 0
        rightPadding: control.indicator.width + control.spacing

        text: control.displayText
        font: control.font
        color: VLCStyle.colors.buttonText
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 120
        implicitHeight: 40
        color: VLCStyle.colors.button
        border.color: control.activeFocus ? VLCStyle.colors.accent : VLCStyle.colors.buttonBorder
        border.width: control.activeFocus ? 2 : 1
        radius: 2
    }

    popup: Popup {
        y: control.height - 1
        width: control.width
        implicitHeight: contentItem.implicitHeight
        padding: 1

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex

            highlight: Rectangle {
                color: VLCStyle.colors.accent
            }

            Rectangle {
                z: 10
                width: parent.width
                height: parent.height
                color: "transparent"
                border.color: VLCStyle.colors.accent
            }

            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: VLCStyle.colors.button
            border.color: VLCStyle.colors.buttonBorder
            radius: 2
        }
    }
}
