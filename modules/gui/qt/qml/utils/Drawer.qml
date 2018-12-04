import QtQuick 2.10
import QtQuick.Controls 2.4

NavigableFocusScope {
    id: root

    property Component component: undefined

    //readonly property int horizontal: 0
    //readonly property int vertical: 1

    property bool expandHorizontally: true
    width:  (root.expandHorizontally) ? root._size : undefined
    height: (!root.expandHorizontally) ? root._size : undefined
    property int _size: 0

    property alias contentItem: content.item

    Flickable {
        anchors.fill: parent
        Loader {
            focus: true
            id: content
            sourceComponent: root.component
        }
    }

    state: "hidden"
    states: [
        State {
            name: "visible"
            PropertyChanges {
                target: root
                _size: (root.expandHorizontally) ?  content.item.width : content.item.height
                visible: true
            }
        },
        State {
            name: "hidden"
            PropertyChanges {
                target: root
                _size: 0
                visible: false
            }
        }
    ]
    transitions: [
        Transition {
            to: "hidden"
            SequentialAnimation {
                NumberAnimation { target: root; property: "_size"; duration: 200 }
                PropertyAction{ target: root; property: "visible" }
            }
        },
        Transition {
            to: "visible"
            SequentialAnimation {
                PropertyAction{ target: root; property: "visible" }
                NumberAnimation { target: root; property: "_size"; duration: 200 }
            }
        }
    ]
}
