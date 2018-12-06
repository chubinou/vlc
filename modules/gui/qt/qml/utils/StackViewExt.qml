import QtQuick 2.10
import QtQuick.Controls 2.3

/*
 * Custom StackView with brief transitions and helper to load view from the history
 */
StackView {
    id: root

    replaceEnter: Transition {
        PropertyAnimation {
            property: "opacity"
            from: 0
            to:1
            duration: 500
        }
    }

    replaceExit: Transition {
        PropertyAnimation {
            property: "opacity"
            from: 1
            to:0
            duration: 500
        }
    }

    /**
     * viewModel: model with the definition of the available view
     *            elemements should contains at least :
     *     name: name of the view
     *     url or component: the url of the Component or the component to load
     * view: string (name of the view to load)
     * viewProperties: map of the propertes to apply to the view
     */
    function loadView(viewModel, view, viewProperties)
    {
        var found = false
        for (var tab = 0; tab < viewModel.length; tab++ )
        {
            var model = viewModel[tab]
            if (model.name === view) {
                //we can't use push(url, properties) as Qt interprets viewProperties
                //as a second component to load
                var component = undefined
                if (model.component) {
                    component = model.component
                } else if ( model.url ) {
                    component = Qt.createComponent(model.url)
                } else {
                    console.warn( "you should define either component or url of the view to load" )
                    return false
                }

                if (component.status === Component.Ready ) {
                    //note doesn't work with qt 5.9, you have to do the following, and beware
                    //that page won 't be released when poped out
                    //var page = component.createObject(root, viewProperties)
                    //root.replace(page)
                    root.replace(null, component, viewProperties)
                    found = true
                    break;
                } else {
                    console.warn("component is not ready")
                }
            }
        }
        if (!found)
            console.warn("unable to load view " + view)
        return found
    }
}
