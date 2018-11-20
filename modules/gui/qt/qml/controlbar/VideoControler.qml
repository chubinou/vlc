import QtQuick 2.9
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3

import org.videolan.vlc 0.1

import "qrc:///style/"
import "qrc:///utils/" as Utils
import "qrc:///playlist/" as PL

Utils.NavigableFocusScope {
    id: root
    focus: true
    height: 80

    signal visibilityChanged(bool visible)

    Rectangle {
        anchors.fill: parent
        color: VLCStyle.colors.setColorAlpha(VLCStyle.colors.banner, 0.9)

        ModalControlBar {
            anchors.fill: parent
            focus: true
            onActionCancel: {
                root.state = "hidden"
            }
        }
    }


    state: "control"
    states: [
        State {
            name: "hidden"
        },
        State {
            name: "control"
        }
    ]
    onStateChanged: {
        root.visibilityChanged( root.state !== "hidden" )
    }

    Keys.priority: Keys.AfterItem
    Keys.onPressed: {
        console.log("root key handle", root.state)
        if (root.state !== "hidden")
            return;
        root.state = "control"
        event.accepted = true
    }
}
