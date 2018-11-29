/*****************************************************************************
 * MCMusicDisplay.qml : The music component of the mediacenter
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

import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "qrc:///utils/" as Utils
import "qrc:///style/"

import org.videolan.medialib 0.1

Utils.NavigableFocusScope {
    id: root

    //name and properties of the tab to be initially loaded
    property string view: "albums"
    property var viewProperties: QtObject {}

    readonly property var pageModel: [
        { name: "albums" , url: "qrc:///mediacenter/MusicAlbumsDisplay.qml"},
        { name: "artists", url: "qrc:///mediacenter/MusicArtistsDisplay.qml"},
        { name: "genres" , url: "qrc:///mediacenter/MusicGenresDisplay.qml"},
        { name: "tracks" , url: "qrc:///mediacenter/MusicTrackListDisplay.qml"}
    ]

    property var tabModel: ListModel {
        ListElement {
            displayText: qsTr("Albums")
            name: "albums"
        }
        ListElement {
            displayText: qsTr("Artists")
            name: "artists"
        }
        ListElement {
            displayText: qsTr("Genres")
            name: "genres"
        }
        ListElement {
            displayText: qsTr("Tracks")
            name: "tracks"
        }
    }

    ColumnLayout {
        anchors.fill : parent

        Utils.NavigableFocusScope {
            id: toobar

            Layout.fillWidth: true
            Layout.preferredHeight: VLCStyle.icon_normal + VLCStyle.margin_small

            Rectangle {
                anchors.fill: parent
                color: VLCStyle.colors.banner

                RowLayout {
                    anchors.fill: parent

                    TabBar {
                        id: bar

                        focus: true

                        Layout.preferredHeight: parent.height - VLCStyle.margin_small
                        Layout.alignment: Qt.AlignVCenter

                        background: Rectangle {
                            color: VLCStyle.colors.banner
                        }
                        Component.onCompleted: {
                            bar.contentItem.focus= true
                        }

                        /* List of sub-sources for Music */
                        Repeater {
                            id: model_music_id

                            model: tabModel

                            //Column {
                            TabButton {
                                id: control
                                text: model.displayText
                                font.pixelSize: VLCStyle.fontSize_normal
                                background: Rectangle {
                                    color: control.hovered ? VLCStyle.colors.bannerHover : VLCStyle.colors.banner
                                }
                                contentItem: Label {
                                    text: control.text
                                    font: control.font
                                    color:  control.hovered ?  VLCStyle.colors.textActiveSource : VLCStyle.colors.text
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter

                                    Rectangle {
                                        anchors {
                                            left: parent.left
                                            right: parent.right
                                            bottom: parent.bottom
                                        }
                                        height: 2
                                        visible: control.activeFocus || control.checked
                                        color: control.activeFocus ? VLCStyle.colors.accent  : VLCStyle.colors.bgHover
                                    }
                                }
                                onClicked: {
                                    stackView.replace(pageModel[index].url)
                                    history.push(["mc", "music", model.name ], History.Stay)
                                    stackView.focus = true
                                }
                                checked: (model.name === root.view)
                                activeFocusOnTab: true
                            }
                        }

                        KeyNavigation.right: searchBox
                    }

                    /* Spacer */
                    Item {
                        Layout.fillWidth: true
                    }

                    TextField {
                        Layout.preferredWidth: VLCStyle.widthSearchInput
                        Layout.preferredHeight: parent.height - VLCStyle.margin_small
                        Layout.alignment: Qt.AlignVCenter  | Qt.AlignRight

                        id: searchBox
                        font.pixelSize: VLCStyle.fontSize_normal

                        color: VLCStyle.colors.buttonText
                        placeholderText: qsTr("filter")
                        hoverEnabled: true

                        background: Rectangle {
                            radius: 5 //fixme
                            color: VLCStyle.colors.button
                            border.color: {
                                if ( searchBox.text.length < 3 && searchBox.text.length !== 0 )
                                    return VLCStyle.colors.alert
                                else if ( searchBox.hovered || searchBox.activeFocus )
                                    return VLCStyle.colors.accent
                                else
                                    return VLCStyle.colors.buttonBorder
                           }
                        }

                        onTextChanged: {
                            stackView.currentItem.model.searchPattern = text;
                        }

                        KeyNavigation.right: combo
                    }

                    /* Selector to choose a specific sorting operation */
                    Utils.ComboBoxExt {
                        id: combo

                        //Layout.fillHeight: true
                        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                        Layout.preferredWidth: VLCStyle.widthSortBox
                        Layout.preferredHeight: parent.height - VLCStyle.margin_small
                        textRole: "text"
                        model: stackView.currentItem.sortModel
                        onCurrentIndexChanged: {
                            var sorting = model.get(currentIndex);
                            stackView.currentItem.model.sortCriteria = sorting.criteria
                        }
                    }
                }
            }

            onActionLeft:   root.actionLeft(index)
            onActionRight:  root.actionRight(index)
            onActionDown:   stackView.focus = true
            onActionUp:     root.actionUp( index )
            onActionCancel: root.actionCancel( index )

            Keys.priority: Keys.AfterItem
            Keys.onPressed: {
                if (!event.accepted)
                    defaultKeyAction(event, 0)
            }
        }

        /* The data elements */
        Utils.StackViewExt  {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true
            focus: true

            Component.onCompleted: {
                var found = stackView.loadView(root.pageModel, view, viewProperties)
                if (!found)
                    push(pageModel[0])
            }
        }

        Connections {
            target: stackView.currentItem
            ignoreUnknownSignals: true
            onActionLeft:   root.actionLeft(index)
            onActionRight:  root.actionRight(index)
            onActionDown:   root.actionDown(index)
            onActionUp:     toobar.focus = true
            onActionCancel: toobar.focus = true
        }
    }
}
