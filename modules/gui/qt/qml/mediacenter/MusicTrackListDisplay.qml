import QtQuick 2.9
import QtQuick.Controls 2.4
import QtQml.Models 2.2
import QtQuick.Layouts 1.3

import org.videolan.medialib 0.1

import "qrc:///utils/" as Utils
import "qrc:///style/"

FocusScope {
    id: root

    //forwarded from subview
    signal actionLeft( int index )
    signal actionRight( int index )
    signal actionCancel( int index )

    property var sortModel: ListModel {
        ListElement{ criteria: "track_number";width:0.15; text: qsTr("TRACK NB"); showSection: "" }
        ListElement{ criteria: "disc_number"; width:0.15; text: qsTr("DISC NB");  showSection: "" }
        ListElement{ criteria: "title";       width:0.15; text: qsTr("TITLE");    showSection: "title" }
        ListElement{ criteria: "main_artist"; width:0.15; text: qsTr("ARTIST");   showSection: "main_artist" }
        ListElement{ criteria: "album_title"; width:0.15; text: qsTr("ALBUM");    showSection: "album_title" }
        ListElement{ criteria: "duration";    width:0.15; text: qsTr("DURATION"); showSection: "" }
    }

    property var model: MLAlbumTrackModel {
        id: rootmodel
        ml: medialib
    }
    property alias parentId: rootmodel.parentId

    property alias contentHeight: view.contentHeight

    property alias interactive: view.interactive

    Utils.SelectableDelegateModel {
        id: delegateModel

        model: root.model

        delegate: Package {
            id: element
            property var rowModel: model

            Rectangle {
                Package.name: "list"
                id: lineView

                width: parent.width
                height: VLCStyle.fontHeight_normal + VLCStyle.margin_xxsmall

                color:  VLCStyle.colors.getBgColor(element.DelegateModel.inSelected, hoverArea.containsMouse, view.currentIndex === index)

                MouseArea {
                    id: hoverArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        delegateModel.updateSelection( mouse.modifiers , view.currentIndex, index)
                        view.currentIndex = rowModel.index
                        lineView.forceActiveFocus()
                    }

                    onDoubleClicked: {
                        medialib.addAndPlay( rowModel.id )
                    }


                    RowLayout {
                        anchors.fill: parent

                        Repeater {
                            model: sortModel

                            Item {
                                Layout.preferredHeight: VLCStyle.fontHeight_normal
                                Layout.preferredWidth: model.width * view.width

                                Text {
                                    text: rowModel[model.criteria]
                                    elide: Text.ElideRight
                                    font.pixelSize: VLCStyle.fontSize_normal
                                    color: VLCStyle.colors.text

                                    anchors {
                                        fill: parent
                                        leftMargin: VLCStyle.margin_xxsmall
                                        rightMargin: VLCStyle.margin_xxsmall
                                    }
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignLeft
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    color: VLCStyle.colors.buttonBorder
                    antialiasing: true
                    anchors{
                        right: parent.right
                        bottom: parent.bottom
                        left: parent    .left
                    }
                    height: 1
                }
            }
        }
    }


    Utils.KeyNavigableListView {
        id: view

        anchors.fill: parent

        highlightMoveVelocity: 1000

        model : delegateModel.parts.list
        modelCount: delegateModel.items.count

        header: Rectangle {
            height: VLCStyle.fontHeight_normal
            width: parent.width
            color: VLCStyle.colors.button

            RowLayout {
                anchors.fill: parent
                Repeater {
                    model: sortModel
                    MouseArea {
                        Layout.preferredHeight: VLCStyle.fontHeight_normal
                        Layout.preferredWidth: model.width * view.width
                        Layout.alignment: Qt.AlignVCenter

                        Text {
                            text: model.text
                            elide: Text.ElideRight
                            font {
                                bold: true
                                pixelSize: VLCStyle.fontSize_normal

                            }
                            color: VLCStyle.colors.buttonText
                            horizontalAlignment: Text.AlignLeft
                            anchors {
                                fill: parent
                                leftMargin: VLCStyle.margin_xxsmall
                                rightMargin: VLCStyle.margin_xxsmall
                            }
                        }

                        Text {
                            text: (rootmodel.sortOrder === Qt.AscendingOrder) ? "▼" : "▲"
                            visible: rootmodel.sortCriteria === model.criteria
                            font.pixelSize: VLCStyle.fontSize_normal
                            color: VLCStyle.colors.accent
                            anchors {
                                right: parent.right
                                leftMargin: VLCStyle.margin_xxsmall
                                rightMargin: VLCStyle.margin_xxsmall
                            }
                        }
                        onClicked: {
                            if (rootmodel.sortCriteria !== model.criteria)
                                rootmodel.sortCriteria = model.criteria
                            else
                                rootmodel.sortOrder = (rootmodel.sortOrder === Qt.AscendingOrder) ? Qt.DescendingOrder : Qt.AscendingOrder
                        }
                    }
                }
            }

            //line below
            Rectangle {
                color: VLCStyle.colors.buttonBorder
                height: 1
                width: parent.width
                anchors.bottom: parent.bottom
            }
        }

        onSelectAll: delegateModel.selectAll()
        onSelectionUpdated: delegateModel.updateSelection( keyModifiers, oldIndex, newIndex )
        onActionLeft: root.actionLeft(index)
        onActionRight: root.actionRight(index)
        onActionCancel: root.actionCancel(index)
    }
}
