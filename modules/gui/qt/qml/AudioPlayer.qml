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
    Item {
        focus: false
        anchors {
            top: parent.top
            right: parent.right
            left: parent.left
            bottom: controllerBarId.top
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

    Utils.NavigableFocusScope {
        id: playlistpopup
        width: 0
        anchors {
            top: parent.top
            right: parent.right
            bottom: controllerBarId.top
        }
        focus: false

        Flickable {
            anchors.fill: parent

            Rectangle {
                color: VLCStyle.colors.setColorAlpha(VLCStyle.colors.banner, 0.9)
                width: root.width/4
                height: parent.height

                PL.PlaylistListView {
                    id: playlistView
                    focus: true
                    anchors.fill: parent
                    onActionLeft: playlistpopup.quit()
                    onActionCancel: playlistpopup.quit()
                }
            }
        }

        function quit() {
            state = "hidden"
            controllerId.focus = true
        }

        state: "hidden"
        states: [
            State {
                name: "visible"
                PropertyChanges {
                    target: playlistpopup
                    width: playlistView.width
                    visible: true
                    focus: true
                }
            },
            State {
                name: "hidden"
                PropertyChanges {
                    target: playlistpopup
                    height: 0
                    visible: false
                    focus: false
                }
            }
        ]
        transitions: [
            Transition {
                to: "hidden"
                SequentialAnimation {
                    NumberAnimation { target: playlistpopup; property: "width"; duration: 200 }
                    PropertyAction{ target: playlistpopup; property: "visible" }
                }
            },
            Transition {
                to: "visible"
                SequentialAnimation {
                    PropertyAction{ target: playlistpopup; property: "visible" }
                    NumberAnimation { target: playlistpopup; property: "width"; duration: 200 }
                }
            }
        ]
    }


    Rectangle {
        id: controllerBarId
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: 80
        color: VLCStyle.colors.banner

        CB.ControlBar {
            id: controllerId
            focus: true
            anchors.fill: parent

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
