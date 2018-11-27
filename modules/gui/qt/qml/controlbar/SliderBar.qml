import QtQuick 2.9
import QtQuick.Controls 2.2

import "qrc:///style/"

Slider {
    id: control
    anchors.margins: VLCStyle.margin_xxsmall

    value: player.position
    onMoved: player.position = control.position

    height: control.barHeight + VLCStyle.fontHeight_normal + VLCStyle.margin_xxsmall * 2
    implicitHeight: control.barHeight + VLCStyle.fontHeight_normal + VLCStyle.margin_xxsmall * 2

    topPadding: 0
    leftPadding: 0
    bottomPadding: 0
    rightPadding: 0

    stepSize: 0.01

    property int barHeight: 5

    background: Rectangle {
        width: control.availableWidth
        implicitHeight: control.implicitHeight
        height: implicitHeight
        color: VLCStyle.colors.banner

        Rectangle {
            width: control.visualPosition * parent.width
            height: control.barHeight
            color: control.activeFocus ? VLCStyle.colors.accent : VLCStyle.colors.bgHover
            radius: control.barHeight
        }

        Text {
            text: player.time.toString()
            color: VLCStyle.colors.text
            font.pixelSize: VLCStyle.fontSize_normal
            anchors {
                bottom: parent.bottom
                bottomMargin: VLCStyle.margin_xxsmall
                left: parent.left
                leftMargin: VLCStyle.margin_xxsmall
            }
        }

        Text {
            text: player.length.toString()
            color: VLCStyle.colors.text
            font.pixelSize: VLCStyle.fontSize_normal
            anchors {
                bottom: parent.bottom
                bottomMargin: VLCStyle.margin_xxsmall
                right: parent.right
                rightMargin: VLCStyle.margin_xxsmall
            }
        }
    }

    handle: Rectangle {
        visible: control.activeFocus
        x: (control.visualPosition * control.availableWidth) - width / 2
        y: (control.barHeight - width) / 2
        implicitWidth: VLCStyle.margin_small
        implicitHeight: VLCStyle.margin_small
        radius: VLCStyle.margin_small
        color: VLCStyle.colors.accent
    }
}
