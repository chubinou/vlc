import QtQuick 2.10
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3
import org.videolan.vlc 0.1

import "qrc:///style/"
import "qrc:///utils/" as Utils

FocusScope {
    id: root
    property alias columnLayout: columnLayout
    signal actionCancel()

    AboutModel {
        id: about
    }

    ButtonGroup {
        buttons: columnLayout.children
    }

    RowLayout {
        id: rowLayout
        anchors.fill: parent

        Rectangle {

            Layout.preferredWidth: columnLayout.implicitWidth
            Layout.fillHeight: true
            color: VLCStyle.colors.banner

            ColumnLayout {
                id: columnLayout
                anchors.fill: parent

                Utils.TextToolButton {
                    id: authorsBtn
                    text: qsTr("Authors")
                    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                    onClicked: {
                        checked = true
                        textArea.text = about.authors
                    }
                    KeyNavigation.down: licenseBtn
                    KeyNavigation.right: textScroll
                    focus: true
                }

                Utils.TextToolButton {
                    id: licenseBtn
                    text: qsTr("License")
                    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                    onClicked: textArea.text = about.license
                    KeyNavigation.down: creditBtn
                    KeyNavigation.right: textScroll
                }

                Utils.TextToolButton {
                    id: creditBtn
                    text: qsTr("Credit")
                    Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                    KeyNavigation.down: backBtn
                    KeyNavigation.right: textScroll

                    onClicked: {
                        checked = true
                        textArea.text = about.thanks
                    }
                }

                Item {
                    Layout.fillHeight: true
                }

                Utils.IconToolButton {
                    id: backBtn
                    size: VLCStyle.icon_large
                    text: VLCIcons.exit
                    Layout.alignment: Qt.AlignLeft | Qt.AlignBottom
                    KeyNavigation.right: textScroll

                    onClicked: {
                        root.actionCancel()
                    }
                }
            }
        }

        ColumnLayout {
            id: columnLayout1
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.alignment:  Qt.AlignHCenter

            Text {
                id: text1
                text: qsTr("VLC Media Player")
                color: VLCStyle.colors.text
                font.pixelSize: VLCStyle.fontSize_xxxlarge
            }


            Text {
                text: about.version
                color: VLCStyle.colors.text
                font.pixelSize: VLCStyle.fontSize_xlarge
            }


            ScrollView {
                id: textScroll
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.alignment:  Qt.AlignHCenter

                Keys.onPressed:  {
                    if (event.key === Qt.Key_Left) {
                        backBtn.focus = true
                        event.accepted = true
                    }
                }

                clip: true

                Text {
                    id: textArea
                    text: about.thanks
                    horizontalAlignment: Text.AlignHCenter
                    anchors.fill: parent
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter

                    color: VLCStyle.colors.text
                    enabled: false
                }
            }
        }
    }

}
