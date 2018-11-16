import QtQuick 2.9
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3

import org.videolan.vlc 0.1

import "qrc:///style/"
import "qrc:///utils/" as Utils
import "qrc:///controlbar/" as CB

Utils.NavigableFocusScope {
    id: root
    anchors.fill: parent
    PlaylistControlerModel {
        id: playlistCtrl
        playlistPtr: mainctx.playlist
    }
    RowLayout {
        id: topBarId
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        Utils.BackButton {
            width: VLCStyle.icon_normal
            height: VLCStyle.icon_normal
            onClicked: playlistCtrl.stop()
        }
    }

    Rectangle {
        focus: false
        color: "black"
        anchors {
            top: topBarId.bottom
            right: parent.right
            left: parent.left
            bottom: controllerId.top
        }
        Rectangle {
            width: parent.width / 2
            height: parent.height / 2
            anchors.centerIn: parent
            Image {
                id: coverId
                source: VLCStyle.noArtCover
                width: parent.width / 2
                height: parent.height / 2
                anchors.centerIn: parent
            }
        }
    }
    CB.AudioController {
        id: controllerId
        focus: true
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: 80
    }
}
