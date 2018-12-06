import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import "qrc:///utils/" as Utils
import "qrc:///style/"

Dialog {
    id: control

    property var rootWindow: undefined

    focus: true
    modal: true

    x: (rootWindow.width - width) / 2
    y: (rootWindow.height - height) / 2
    padding: VLCStyle.margin_normal
    margins: VLCStyle.margin_large

    Overlay.modal: GaussianBlur {
        source: ShaderEffectSource {
            sourceItem: control.rootWindow
            live: true
        }
        radius: 12
        samples: 16
    }

    background: Rectangle {
        color: VLCStyle.colors.banner
    }

    header: Label {
        text: control.title
        visible: control.title
        elide: Label.ElideRight
        font.bold: true
        color: VLCStyle.colors.text
        padding: 6
        background: Rectangle {
            x: 1; y: 1
            width: parent.width - 2
            height: parent.height - 1
            color: VLCStyle.colors.banner
        }
    }

    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0 }
    }
    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0 }
    }
}
