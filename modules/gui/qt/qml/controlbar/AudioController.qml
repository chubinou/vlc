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

    signal visibilityChanged(bool visible)




    Utils.NavigableFocusScope {
        id: controlbarpopup
        focus: true
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: controlbar.height


        ControlBar {
            id: controlbar
            width: parent.width
            height: 80
            focus: true
            onActionCancel: {
                controlbarpopup.state = "hidden"
            }
            onShowPlaylist: {
                if (playlistpopup.state === "visible")
                    playlistpopup.state = "hidden"
                else
                    playlistpopup.state = "visible"
                playlistpopup.focus = true
            }
        }

        state: "visible"
        onStateChanged: {
            root.visibilityChanged( state === "visible" )
        }

        states: [
            State {
                name: "visible"
                PropertyChanges {
                    target: controlbarpopup
                    height: controlbar.height
                    visible: true
                    focus: true
                }
            },
            State {
                name: "hidden"
                PropertyChanges {
                    target: controlbarpopup
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
                    NumberAnimation { target: controlbarpopup; property: "height"; duration: 200 }
                    PropertyAction{ target: controlbarpopup; property: "visible" }
                }
            },
            Transition {
                to: "visible"
                SequentialAnimation {
                    PropertyAction{ target: controlbarpopup; property: "visible" }
                    NumberAnimation { target: controlbarpopup; property: "height"; duration: 200 }
                }
            }
        ]

    }

    Keys.priority: Keys.BeforeItem
    Keys.onPressed: {
        console.log("root key handle", controlbarpopup.state)
        if (controlbarpopup.state === "visible")
            return;
        controlbarpopup.state = "visible"
        controlbarpopup.focus = true
        event.accepted = true
    }
}
