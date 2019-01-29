#ifndef VLC_QT_VIDEORENDERERWL_HPP
#define VLC_QT_VIDEORENDERERWL_HPP

#include <inttypes.h>
#include <QtQuick/QQuickItem>
#include <QMutex>
#include <QtQuick/QSGRectangleNode>
#include <components/qml_main_context.hpp>
#include "qt.hpp"

#include "qvoutwindow.hpp"

class MainInterface;
class VideoSurfaceWayland;

struct wl_compositor;
struct wl_subcompositor;
struct wl_subsurface;
struct wl_registry;

class QVoutWindowWayland: public QVoutWindow
{
    Q_OBJECT
public:
    QVoutWindowWayland(MainInterface* p_mi,  QObject *parent = nullptr);
    ~QVoutWindowWayland() override;

    void enableVideo(unsigned int width, unsigned int height, bool fullscreen) override;
    void disableVideo() override;

    VideoSurfaceProvider *getVideoSurfaceProvider() override;

signals:
    void updated();

private:
    /* wayland registry callbacks */
    static void registry_global( void *, wl_registry *, uint32_t,
                                 const char *, uint32_t );
    static void registry_global_remove( void *, wl_registry *, uint32_t );

    MainInterface *m_mainInterface;
    VideoSurfaceWayland *m_surfaceProvider = nullptr;
    wl_compositor *m_compositor = nullptr;
    wl_subcompositor *m_subcompositor = nullptr;
    wl_subsurface *m_subsurface = nullptr;
    wl_surface *m_surface = nullptr;
};

class VideoSurfaceWayland : public VideoSurfaceProvider
{
    Q_OBJECT
public:
    VideoSurfaceWayland(QVoutWindowWayland* renderer, QObject* parent = nullptr);

private:
    QSGNode *updatePaintNode(QQuickItem* item, QSGNode *, QQuickItem::UpdatePaintNodeData *) override;

private:
    QVoutWindowWayland* m_renderer = nullptr;
};

#endif // VLC_QT_VIDEORENDERERWL_HPP
