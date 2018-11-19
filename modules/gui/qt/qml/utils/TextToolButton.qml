import QtQuick 2.10
import QtQuick.Controls 2.4

import "qrc:///style/"

ToolButton {
    id: control

    contentItem: Label {
        text: control.text
        font: control.font
        color: VLCStyle.colors.text
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        anchors {
            verticalCenter: parent.verticalCenter
            rightMargin: VLCStyle.margin_xsmall
            leftMargin: VLCStyle.margin_small
        }

        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            height: 2
            visible: control.activeFocus || control.checked
            color: control.activeFocus ? VLCStyle.colors.accent : VLCStyle.colors.bgHover
        }
    }

    background: Rectangle {
        implicitHeight: parent.height
        width: parent.contentItem.width
        color: "transparent"
    }


    Keys.onReturnPressed: {
        control.clicked()
    }
}
