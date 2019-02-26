#ifndef VIDEORENDERER_HPP
#define VIDEORENDERER_HPP

#include <QObject>
#include <QMutex>
#include "qt.hpp"
#include "vlc_vout_window.h"
#include "videosurface.hpp"
#include <QtQuickWidgets//QQuickWidget>

class QVoutWindow : public QObject
{
    Q_OBJECT
public:
    QVoutWindow(MainInterface* p_mi);
    virtual ~QVoutWindow();

    virtual void setupVoutWindow(vout_window_t* window);
    virtual void enableVideo(unsigned width, unsigned height, bool fullscreen);
    virtual void disableVideo();
    virtual void windowClosed();

    virtual VideoSurfaceProvider* getVideoSurfaceProvider() = 0;

    /*
     * Create the main QML view and parent it to the main window
     * This can be overloaded for special needs (ie: draw the interface in an offscreen window)
     */
    virtual QQuickWidget* createQuickWindow();

public slots:
    void onMousePressed( int vlcButton );
    void onMouseReleased( int vlcButton );
    void onMouseDoubleClick( int vlcButton );
    void onMouseMoved( float x, float y );
    void onMouseWheeled(const QPointF &pos, int delta, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers, Qt::Orientation orient);
    void onKeyPressed(int key, Qt::KeyboardModifiers modifiers);
    void onSurfaceSizeChanged(QSizeF size);

protected:
    QMutex m_voutlock;
    vout_window_t* m_voutWindow = nullptr;
    MainInterface* m_mainInterface = nullptr;
    bool m_hasVideo = false;

    QSizeF m_surfaceSize;

};

#endif // VIDEORENDERER_HPP
