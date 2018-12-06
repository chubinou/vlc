import QtQuick 2.9
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import org.videolan.vlc 0.1

import "qrc:///style/"
import "qrc:///utils/" as Utils
import "qrc:///controlbar/" as CB
import "qrc:///playlist/" as PL

Utils.NavigableFocusScope {
    id: root

    PlaylistControlerModel {
        id: playlistCtrl
        playlistPtr: mainctx.playlist
    }

    //center image
    Rectangle {
        visible: !player.hasVideoOutput
        focus: false
        color: VLCStyle.colors.bg
        anchors {
            top: parent.top
            right: parent.right
            left: parent.left
            bottom: controlBarView.top
        }

        Image {
            id: cover
            source: (playlistCtrl.currentItem.artwork && playlistCtrl.currentItem.artwork.toString())
                    ? playlistCtrl.currentItem.artwork
                    : VLCStyle.noArtCover
            fillMode: Image.PreserveAspectFit
            width: parent.width / 2
            height: parent.height / 2
            anchors.centerIn: parent
        }

        DropShadow {
            anchors.fill: cover
            source: cover
            horizontalOffset: 3
            verticalOffset: 10
            radius: 12
            samples: 17
            color: "black"
        }
    }

    Utils.Drawer {
        id: playlistpopup
        anchors {
            top: parent.top
            right: parent.right
            bottom: controlBarView.top
        }
        focus: false
        expandHorizontally: true

        component: Rectangle {
            color: VLCStyle.colors.setColorAlpha(VLCStyle.colors.banner, 0.9)
            width: root.width/4
            height: playlistpopup.height

            PL.PlaylistListView {
                id: playlistView
                focus: true
                anchors.fill: parent
                onActionLeft: playlistpopup.quit()
                onActionCancel: playlistpopup.quit()
            }
        }

        function quit() {
            state = "hidden"
            controlBarView.focus = true
        }
        onStateChanged: {
            if (state === "hidden")
                toolbarAutoHide.restart()
        }
    }


    Utils.Drawer {
        id: controlBarView
        focus: true
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        property var  noAutoHide: controlBarView.contentItem.noAutoHide

        state: "visible"
        expandHorizontally: false

        component: Rectangle {
            id: controllerBarId
            color: VLCStyle.colors.banner
            width: controlBarView.width
            height: 90 * VLCStyle.scale
            property alias noAutoHide: controllerId.noAutoHide

            ModalControlBar {
                id: controllerId
                focus: true
                anchors.fill: parent

                forceNoAutoHide: playlistpopup.state === "visible" || !player.hasVideoOutput
                onNoAutoHideChanged: {
                    if (!noAutoHide)
                        toolbarAutoHide.restart()
                }
                showPlaylistButton: true

                onActionUp: root.actionUp(index)
                onActionDown: root.actionDown(index)
                onActionLeft: root.actionLeft(index)
                onActionRight: root.actionRight(index)
                onActionCancel: root.actionCancel(index)
                onShowPlaylist: {
                    if (playlistpopup.state === "visible") {
                        playlistpopup.state = "hidden"
                    } else {
                        playlistpopup.state = "visible"
                        playlistpopup.focus = true
                    }
                }
            }
        }
    }

    Timer {
        id: toolbarAutoHide
        running: true
        repeat: false
        interval: 5000
        onTriggered: {
            if (controlBarView.noAutoHide)
                return;
            controlBarView.state = "hidden"
        }

        function setVisible() {
            controlBarView.state = "visible"
            toolbarAutoHide.restart()
        }
    }

    Component.onCompleted: {
        filter.source = rootWindow
    }

    EventFilter {
        id: filter
        filterEnabled: true

        //only handle key release to avoid underneath button to be
        //triggered as it gains focus between events
        Keys.onReleased: {
            if (controlBarView.state === "hidden")
                event.accepted = true
            toolbarAutoHide.setVisible()
        }
    }

}
