/*****************************************************************************
 * MainInterface.qml : Main QML component displaying the mediacenter, the
 *     playlist and the sources selection
 ****************************************************************************
 * Copyright (C) 2006-2011 VideoLAN and AUTHORS
 * $Id$
 *
 * Authors: Maël Kervella <dev@maelkervella.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

import QtQuick 2.10
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4
import org.videolan.medialib 0.1
import org.videolan.vlc 0.1

import "qrc:///utils/" as Utils
import "qrc:///style/"

import "qrc:///mediacenter/" as MC
import "qrc:///playlist/" as PL
import "qrc:///controlbar/" as CB

Rectangle {
    id: root
    color: VLCStyle.colors.bg

    Component {
        id: medialibComp
        FocusScope {
            focus: true
            Row {
                anchors.fill: parent
                MC.MCMainDisplay {
                    id: medialibId
                    onActionRight: playlist.focus = true
                    focus: true
                    width: parent.width * (2. / 3)
                    height: parent.height
                }

                PL.PlaylistListView {
                    id: playlist
                    focus: false
                    width: parent.width / 3
                    height: parent.height
                    onActionLeft: medialibId.focus = true
                }
            }
        }
    }

    Component {
        id: audioplayerComp
        AudioPlayer {
            focus: true
        }
    }


    StackView {
        id: mainStackViewId
        anchors.fill: parent

        initialItem: medialibComp
        focus: true
        property int prevPlayerState: PlayerControler.PLAYING_STATE_STOPPED


        Connections {
            target: player
            onPlayingStateChanged: {
                if (state == PlayerControler.PLAYING_STATE_STOPPED )
                    mainStackViewId.replace(medialibComp)
                else if (mainStackViewId.prevPlayerState == PlayerControler.PLAYING_STATE_STOPPED)
                    mainStackViewId.replace(audioplayerComp)
                mainStackViewId.prevPlayerState = state
            }
        }

        replaceEnter: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 0
                to:1
                duration: 200
            }
        }

        replaceExit: Transition {
            PropertyAnimation {
                property: "opacity"
                from: 1
                to:0
                duration: 200
            }
        }
    }

    MC.ScanProgressBar {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }

    Component.onCompleted: {
        //set the initial view
        history.push({
                    view : "music",
                    viewProperties : {
                        view : "albums",
                        viewProperties : {}
                    }
                }, History.Go)
    }
}
