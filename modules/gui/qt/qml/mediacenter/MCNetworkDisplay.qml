/*****************************************************************************
 * MCVideoDisplay.qml : The video component of the mediacenter
 ****************************************************************************
 * Copyright (C) 2006-2011 VideoLAN and AUTHORS
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

import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQml.Models 2.2

import org.videolan.medialib 0.1

import "qrc:///utils/" as Utils
import "qrc:///style/"

Utils.NavigableFocusScope {
    id: root

    property string mrl

    Utils.SelectableDelegateModel {
        id: delegateModel
        model: networkModelFactory.create(mainctx, mrl)

        delegate: Package {
            id: element
            Loader {
                focus: true
                Package.name: "list"
                source: model.type == MLNetworkModel.TYPE_FILE ?
                            "qrc:///mediacenter/NetworkFileDisplay.qml" :
                            "qrc:///mediacenter/NetworkDriveDisplay.qml";
            }
        }
        function actionAtIndex(index) {
            if ( delegateModel.selectedGroup.count > 1 ) {
                var list = []
                for (var i = 0; i < delegateModel.selectedGroup.count; i++) {
                    var itemModel = delegateModel.selectedGroup.get(i).model;
                    if (itemModel.type == MLNetworkModel.TYPE_FILE)
                        list.push(itemModel.mrl)
                }
                medialib.addAndPlay( list )
            } else if (delegateModel.selectedGroup.count === 1) {
                var itemModel = delegateModel.selectedGroup.get(0).model;
                if (itemModel.type != MLNetworkModel.TYPE_FILE) {
                    history.push({
                        view: "network",
                        viewProperties: {
                            mrl: itemModel.mrl
                         },
                    }, History.Go);
                } else {
                    medialib.addAndPlay( itemModel.mrl );
                }
            }
        }
    }

    Utils.KeyNavigableListView {
        anchors.fill: parent
        model: delegateModel.parts.list
        modelCount: delegateModel.items.count

        focus: true
        spacing: VLCStyle.margin_xxxsmall

        onSelectAll: delegateModel.selectAll()
        onSelectionUpdated: delegateModel.updateSelection( keyModifiers, oldIndex, newIndex )
        onActionAtIndex: delegateModel.actionAtIndex(index)

        onActionLeft: root.actionLeft(index)
        onActionRight: root.actionRight(index)
        onActionDown: root.actionDown(index)
        onActionUp: root.actionUp(index)
        onActionCancel: root.actionCancel(index)
    }
}
