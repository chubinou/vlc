import QtQuick 2.7
import QtQuick.Controls 2.2

NavigableFocusScope {
    id: listview_id

    property int modelCount: 0

    signal selectionUpdated( int keyModifiers, int oldIndex,int newIndex )
    signal selectAll()
    signal actionAtIndex( int index )

    //here to keep the same interface as GridView
    function shiftX( index ) { return 0 }

    //forward view properties
    property alias spacing: view.spacing
    property alias interactive: view.interactive
    property alias model: view.model
    property alias delegate: view.delegate

    property alias originX: view.originX
    property alias originY: view.originY

    property alias contentX: view.contentX
    property alias contentY:  view.contentY
    property alias contentHeight: view.contentHeight

    property alias footer: view.footer
    property alias footerItem: view.footerItem
    property alias header: view.header
    property alias headerItem: view.headerItem

    property alias currentIndex: view.currentIndex

    property alias highlightMoveVelocity: view.highlightMoveVelocity

    ListView {
        id: view
        anchors.fill: parent
        //key navigation is reimplemented for item selection
        keyNavigationEnabled: false

        focus: true

        clip: true
        ScrollBar.vertical: ScrollBar { id: scroll_id }

        highlightMoveDuration: 300 //ms
        highlightMoveVelocity: 1000 //px/s

        Connections {
            target: view.currentItem
            ignoreUnknownSignals: true
            onActionRight: listview_id.actionRight(currentIndex)
            onActionLeft: listview_id.actionLeft(currentIndex)
            onActionDown: {
                if ( currentIndex !== modelCount - 1 ) {
                    var newIndex = currentIndex + 1
                    var oldIndex = currentIndex
                    currentIndex = newIndex
                    selectionUpdated(0, oldIndex, newIndex)
                } else {
                    root.actionDown(currentIndex)
                }
            }
            onActionUp: {
                if ( currentIndex !== 0 ) {
                    var newIndex = currentIndex - 1
                    var oldIndex = currentIndex
                    currentIndex = newIndex
                    selectionUpdated(0, oldIndex, newIndex)
                } else {
                    root.actionUp(currentIndex)
                }
            }
        }

        Keys.onPressed: {
            var newIndex = -1
            if ( event.key === Qt.Key_Down || event.matches(StandardKey.MoveToNextLine) ||event.matches(StandardKey.SelectNextLine) ) {
                if (currentIndex !== modelCount - 1 )
                    newIndex = currentIndex + 1
            } else if ( event.key === Qt.Key_PageDown || event.matches(StandardKey.MoveToNextPage) ||event.matches(StandardKey.SelectNextPage)) {
                newIndex = Math.min(modelCount - 1, currentIndex + 10)
            } else if ( event.key === Qt.Key_Up || event.matches(StandardKey.MoveToPreviousLine) ||event.matches(StandardKey.SelectPreviousLine) ) {
                if ( currentIndex !== 0 )
                    newIndex = currentIndex - 1
            } else if ( event.key === Qt.Key_PageUp || event.matches(StandardKey.MoveToPreviousPage) ||event.matches(StandardKey.SelectPreviousPage)) {
                newIndex = Math.max(0, currentIndex - 10)
            }

            if (newIndex != -1) {
                var oldIndex = currentIndex
                currentIndex = newIndex
                event.accepted = true
                selectionUpdated(event.modifiers, oldIndex, newIndex)
            }

            if (!event.accepted)
                defaultKeyAction(event, currentIndex)
        }

        Keys.onReleased: {
            if (event.matches(StandardKey.SelectAll)) {
                event.accepted = true
                selectAll()
            } else if (event.key === Qt.Key_Space || event.matches(StandardKey.InsertParagraphSeparator)) { //enter/return/space
                event.accepted = true
                actionAtIndex(currentIndex)
            }
        }
    }
}
