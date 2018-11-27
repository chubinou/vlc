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

    Utils.NavigableFocusScope {
        id: playlistpopup
        width: 0
        anchors {
            top: parent.top
            right: parent.right
            bottom: controlBarView.top
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
            controlBarView.focus = true
        }

        state: "hidden"
        onStateChanged: {
            if (state === "hidden")
                toolbarAutoHide.restart()
        }
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
                    width: 0
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

    Utils.NavigableFocusScope {
        id: controlBarView
        focus: true
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: controllerBarId.height
        property alias  noAutoHide: controllerId.noAutoHide

        Flickable {
            anchors.fill: parent

            Rectangle {
                id: controllerBarId
                color: VLCStyle.colors.banner
                width: parent.width
                height: 90

                CB.ModalControlBar {
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

        state: "visible"
        states: [
            State {
                name: "visible"
                PropertyChanges {
                    target: controlBarView
                    height: controllerId.height
                    visible: true
                    focus: true
                }
            },
            State {
                name: "hidden"
                PropertyChanges {
                    target: controlBarView
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
                    NumberAnimation { target: controlBarView; property: "height"; duration: 200 }
                    PropertyAction{ target: controlBarView; property: "visible" }
                }
            },
            Transition {
                to: "visible"
                SequentialAnimation {
                    PropertyAction{ target: controlBarView; property: "visible" }
                    NumberAnimation { target: controlBarView; property: "height"; duration: 200 }
                }
            }
        ]
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
    }

    Keys.priority: Keys.BeforeItem
    Keys.onPressed: {
        controlBarView.state = "visible"
        toolbarAutoHide.restart()
    }
}
