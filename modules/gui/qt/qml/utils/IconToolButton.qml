import QtQuick 2.10
import QtQuick.Controls 2.4

import "qrc:///style/"

ToolButton {
    id: control
    property var color: control.checked
                        ? (control.activeFocus ? VLCStyle.colors.accent : VLCStyle.colors.bgHover )
                        : VLCStyle.colors.buttonText
    property int size: VLCStyle.icon_normal


    contentItem: Label {
        text: control.text
        color: control.color

        font.pixelSize: control.size
        font.family: VLCIcons.fontFamily

        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        anchors {
            centerIn: parent
            //verticalCenter: parent.verticalCenter
            //rightMargin: VLCStyle.margin_xsmall
            //leftMargin: VLCStyle.margin_small
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
        implicitHeight: control.size
        implicitWidth: control.size
        color: "transparent"
    }

    Keys.onReturnPressed: {
        control.clicked()
    }
}
