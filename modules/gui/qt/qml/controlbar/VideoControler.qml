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

    ControlBar {
        id: controlbar
        width: parent.width
        anchors.fill: parent
        focus: root.state === "visible"
        onActionCancel: {
            root.state = "hidden"
        }
    }

    state: "visible"
    onStateChanged: {
        root.visibilityChanged( root.state === "visible" )
    }


    Keys.priority: Keys.AfterItem
    Keys.onPressed: {
        console.log("root key handle", root.state)
        if (root.state === "visible")
            return;
        root.state = "visible"
        event.accepted = true
    }
}
