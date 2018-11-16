import QtQuick 2.9
import QtQuick.Layouts 1.3
// To access `history`
import org.videolan.medialib 0.1

import "qrc:///style/"
import "qrc:///utils/" as Utils

Utils.ImageToolButton {
    id: history_back
    imageSource: "qrc:///toolbar/dvd_prev.svg"

    focus: true

    Layout.preferredHeight: VLCStyle.icon_normal
    Layout.preferredWidth: VLCStyle.icon_normal
    Layout.alignment: Qt.AlignVCenter

    onClicked: history.pop(History.Go)
}
