#include "qvoutwindowwayland.hpp"
#include <QtQuick/QSGImageNode>
#include <QtQuick/QSGRectangleNode>
#include <QtQuick/QQuickWindow>
#include <vlc_vout_window.h>
#include "main_interface.hpp"

#include QPNI_HEADER
#include <wayland-client.h>
#include <wayland-client-protocol.h>

void QVoutWindowWayland::registry_global( void *data, wl_registry *registry,
                                       uint32_t name, const char *intf,
                                       uint32_t version )
{
    QVoutWindowWayland *renderer = static_cast<QVoutWindowWayland*>( data );

    /* For compositor, we try to require version 3 so as to have buffer_scale */
    if( !strcmp(intf, "wl_compositor") )
        renderer->m_compositor = static_cast<wl_compositor*>(
                                 wl_registry_bind( registry, name,
                                                   &wl_compositor_interface,
                                                   __MIN(3, version) ));

    else
    if( !strcmp(intf, "wl_subcompositor") )
        renderer->m_subcompositor = static_cast<wl_subcompositor*>(
                                    wl_registry_bind( registry, name,
                                                      &wl_subcompositor_interface,
                                                      1 ));
}

void QVoutWindowWayland::registry_global_remove( void *, wl_registry *, uint32_t )
{
    // nothing to do
}

QVoutWindowWayland::QVoutWindowWayland(MainInterface* p_mi)
    : QVoutWindow( p_mi )
{
    assert( m_mainInterface );
    m_surfaceProvider = new VideoSurfaceWayland( this, this );
}

QVoutWindowWayland::~QVoutWindowWayland()
{
    if( m_subsurface )
        wl_subsurface_destroy( m_subsurface );
    if( m_surface )
        wl_surface_destroy( m_surface );
    if( m_compositor )
        wl_compositor_destroy( m_compositor );
    if( m_subcompositor )
        wl_subcompositor_destroy( m_subcompositor );
}

void QVoutWindowWayland::enableVideo(unsigned width, unsigned height, bool fullscreen)
{
    QWindow *root_window = nullptr;
    wl_registry *registry = nullptr;
    wl_surface *root_surface = nullptr;
    wl_region *region = nullptr;

    QVoutWindow::enableVideo(width, height, fullscreen);
    if ( !m_hasVideo ) //no window out has been set
        return;

    m_voutWindow->type = VOUT_WINDOW_TYPE_WAYLAND;

    QPlatformNativeInterface *qni = qApp->platformNativeInterface();

    m_voutWindow->display.wl = static_cast<wl_display *>(
         qni->nativeResourceForIntegration(QByteArrayLiteral("wl_display")));

    const wl_registry_listener registry_cbs =
    {
        registry_global,
        registry_global_remove,
    };

    registry = wl_display_get_registry( m_voutWindow->display.wl );
    if( registry == NULL )
        goto error;

    wl_registry_add_listener( registry, &registry_cbs, this );
    wl_display_roundtrip( m_voutWindow->display.wl );
    wl_registry_destroy( registry );

    if( m_compositor == NULL || m_subcompositor == NULL )
        goto error;

    root_window = m_mainInterface->window()->windowHandle();
    root_surface = static_cast<wl_surface *>(
        qni->nativeResourceForWindow( QByteArrayLiteral("surface"),
                                      root_window ));

    m_surface =
    m_voutWindow->handle.wl = wl_compositor_create_surface( m_compositor );
    if( m_voutWindow->handle.wl == NULL )
        goto error;
    
    m_subsurface = wl_subcompositor_get_subsurface( m_subcompositor,
                                                    m_voutWindow->handle.wl,
                                                    root_surface );
    if ( m_subsurface == NULL )
        goto error;

    wl_subsurface_place_below( m_subsurface, root_surface );
    wl_subsurface_set_position( m_subsurface, root_window->frameMargins().left(), root_window->frameMargins().top());
    wl_subsurface_set_desync( m_subsurface );

    /* HACK: disable event input on surface, so that Qt don't try
     * to cast it's userdata. The Qt VideoSurface will get the events
     * instead */
    region = wl_compositor_create_region( m_compositor );
    if( region == NULL )
        goto error;
    wl_region_add( region, 0, 0, 0, 0 );
    wl_surface_set_input_region( m_voutWindow->handle.wl, region );
    wl_region_destroy( region );

    return;

error:
    if( m_subsurface )
        wl_subsurface_destroy(m_subsurface);

    if( m_surface )
        wl_surface_destroy(m_surface);

    return;
}


void QVoutWindowWayland::disableVideo()
{
    QVoutWindow::disableVideo();
    if( m_subsurface )
        wl_subsurface_destroy( m_subsurface );
    if( m_surface )
        wl_surface_destroy( m_surface );
    if( m_subcompositor )
        wl_subcompositor_destroy( m_subcompositor );
    if( m_compositor )
        wl_compositor_destroy( m_compositor );
    m_subsurface = nullptr; m_surface = nullptr;
    m_compositor = nullptr; m_subcompositor = nullptr;
}

VideoSurfaceProvider *QVoutWindowWayland::getVideoSurfaceProvider()
{
    return m_surfaceProvider;
}

VideoSurfaceWayland::VideoSurfaceWayland(QVoutWindowWayland* renderer, QObject* parent)
    : VideoSurfaceProvider( parent )
    , m_renderer(renderer)
{
    connect(this, &VideoSurfaceWayland::mouseMoved, m_renderer, &QVoutWindow::onMouseMoved, Qt::QueuedConnection);
    connect(this, &VideoSurfaceWayland::mousePressed, m_renderer, &QVoutWindow::onMousePressed, Qt::QueuedConnection);
    connect(this, &VideoSurfaceWayland::mouseDblClicked, m_renderer, &QVoutWindow::onMouseDoubleClick, Qt::QueuedConnection);
    connect(this, &VideoSurfaceWayland::mouseReleased, m_renderer, &QVoutWindow::onMouseReleased, Qt::QueuedConnection);
    connect(this, &VideoSurfaceWayland::mouseWheeled, m_renderer, &QVoutWindow::onMouseWheeled, Qt::QueuedConnection);
    connect(this, &VideoSurfaceWayland::keyPressed, m_renderer, &QVoutWindow::onKeyPressed, Qt::QueuedConnection);

    connect(this, &VideoSurfaceWayland::surfaceSizeChanged, m_renderer, &QVoutWindow::onSurfaceSizeChanged);

    connect(m_renderer, &QVoutWindowWayland::updated, this, &VideoSurfaceWayland::update, Qt::QueuedConnection);
}

QSGNode* VideoSurfaceWayland::updatePaintNode(QQuickItem* item, QSGNode* oldNode, QQuickItem::UpdatePaintNodeData*)
{
    QSGRectangleNode* node = static_cast<QSGRectangleNode*>(oldNode);

    if (!node)
    {
        node = item->window()->createRectangleNode();
        node->setColor(Qt::transparent);
    }
    node->setRect(item->boundingRect());

    return node;
}
