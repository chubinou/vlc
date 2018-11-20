/*****************************************************************************
 * NetworkDriveDisplay.qml : Delegate to display a network drive
 ****************************************************************************
 * Copyright (C) 2018 VideoLAN and AUTHORS
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

Utils.NetworkListItem {
    property string _mrl: model.mrl

    width: root.width
    height: VLCStyle.icon_normal

    focus: true

    color: VLCStyle.colors.getBgColor(element.DelegateModel.inSelected, this.hovered, this.activeFocus)

    cover: Image {
        id: cover_obj
        fillMode: Image.PreserveAspectFit
        source: model.type == MLNetworkModel.TYPE_SHARE ?
            "qrc:///type/network.svg" : "qrc:///type/directory.svg";
    }
    line1: model.name || qsTr("Unknown share")
    line2: model.protocol


    onItemClicked : {
        delegateModel.updateSelection( modifier, view.currentItem.currentIndex, index )
        view.currentItem.currentIndex = index
        this.forceActiveFocus()
    }
    onItemDoubleClicked: {
        history.push({
            view: "network",
            viewProperties: {
                mrl: _mrl
             },
        }, History.Go)
    }
    onIndexClicked: {
        model.indexed = !model.indexed;
    }
}
