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

        ControlBar {
            id: controlbar
            anchors.fill: parent
            focus: true
            onShowTrackBar: {
                root.state = "tracks"
            }
            onActionCancel: {
                root.state = "hidden"
            }
        }

        TrackSelector {
            id: trackbar
            anchors.fill: parent
            onActionCancel: {
                root.state = "control"
            }
        }
    }


    state: "control"
    states: [
        State {
            name: "hidden"
            PropertyChanges {
                target: controlbar
                visible: false
                focus: false
            }
            PropertyChanges {
                target: trackbar
                visible: false
                focus: false
            }
        },
        State {
            name: "control"
            PropertyChanges {
                target: controlbar
                visible: true
                focus: true
            }
            PropertyChanges {
                target: trackbar
                visible: false
                focus: false
            }
        },
        State {
            name: "tracks"
            PropertyChanges {
                target: controlbar
                visible: false
                focus: false
            }
            PropertyChanges {
                target: trackbar
                visible: true
                focus: true
            }
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
