import QtQuick 2.10
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3

import org.videolan.vlc 0.1

import "qrc:///style/"
import "qrc:///utils/" as Utils

Utils.NavigableFocusScope {
    id: root
    signal showPlaylist();

    property bool showPlaylistButton: false

    property bool forceNoAutoHide: false
    property bool noAutoHide: state !== "control" || forceNoAutoHide

    Component {
        id: controlbarComp_id
        ControlBar {
            focus: true

            showPlaylistButton: root.showPlaylistButton

            onShowTrackBar: root.state = "tracks"
            onShowPlaylist: root.showPlaylist()

            onActionUp: root.actionUp(index)
            onActionDown: root.actionDown(index)
            onActionLeft: root.actionLeft(index)
            onActionRight: root.actionRight(index)
            onActionCancel: root.actionCancel(index)
        }
    }

    Component {
        id: trackbarComp_id
        TrackSelector {
            focus: true
            onActionCancel:  root.state = "control"

            onActionUp: root.actionUp(index)
            onActionDown: root.actionDown(index)
            onActionLeft: root.actionLeft(index)
            onActionRight: root.actionRight(index)
        }
    }

    Utils.StackViewExt {
        id: stack_id
        initialItem: controlbarComp_id
        anchors.fill: parent
        focus: true
    }

    state: "control"
    onStateChanged: {
        if (state === "tracks")
            stack_id.replace(trackbarComp_id)
        else if (state === "control")
            stack_id.replace(controlbarComp_id)
    }
}
