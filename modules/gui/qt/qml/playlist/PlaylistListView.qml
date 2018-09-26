/*****************************************************************************
 * PlaylistListView.qml : List view that can group similar items
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

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQml.Models 2.2
import org.videolan.medialib 0.1
import org.videolan.vlc 0.1

import "qrc:///utils/" as Utils
import "qrc:///style/"

FocusScope {
    id: container
    Item {
        id: root
        anchors.fill: parent

        property var plmodel: PlaylistListModel {
            playlistId: mainctx.playlist
        }

        Rectangle {
            id: dragItem

            z: 1
            width:  label.implicitWidth
            height: label.implicitHeight
            color: VLCStyle.colors.button
            border.color : VLCStyle.colors.buttonBorder
            visible: false

            Drag.active: visible

            function updatePos(x, y) {
                var pos = root.mapFromGlobal(x, y)
                dragItem.x = pos.x + 10
                dragItem.y = pos.y + 10
            }

            Text {
                id: label
                font.pixelSize: VLCStyle.fontSize_normal
                color: VLCStyle.colors.text
                text: qsTr("%1 tracks selected").arg(delegateModel.selectedGroup.count)
            }
        }

        Utils.SelectableDelegateModel {
            id: delegateModel
            model: root.plmodel

            delegate: Package {
                id: element

                PLItem {
                    Package.name: "list"
                    width: root.width
                    color: VLCStyle.colors.getBgColor(element.DelegateModel.inSelected, this.hovered , this.activeFocus)

                    dragitem: dragItem

                    onItemClicked : {
                        view.forceActiveFocus()
                        delegateModel.updateSelection( modifier , view.currentIndex, index)
                        view.currentIndex = index
                    }
                    onItemDoubleClicked: console.log("itemDoubleClicked")
                    onDropedMovedAt: {
                        var list = []
                        for (var i = 0; i < delegateModel.selectedGroup.count; i++ ) {
                            list.push(delegateModel.selectedGroup.get(i).itemsIndex)
                        }
                        console.log("move", list, "to", target)
                        root.plmodel.moveItems(list, target)
                    }
                }
            }
        }

        Utils.KeyNavigableListView {
            id: view

            focus: true
            anchors.fill: parent
            model: delegateModel.parts.list

            onSelectAll: delegateModel.selectAll()
            onSelectionUpdated: delegateModel.updateSelection( keyModifiers, oldIndex, newIndex )

            Keys.onDeletePressed: {
                var list = []
                for (var i = 0; i < delegateModel.selectedGroup.count; i++ ) {
                    list.push(delegateModel.selectedGroup.get(i).itemsIndex)
                }
                console.log("delete", list)
                root.plmodel.removeItems(list)
            }
        }
    }
}
