#ifndef VIDEORENDERERGL_HPP
#define VIDEORENDERERGL_HPP

#include <QtQuick/QQuickItem>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOffscreenSurface>
#include <QMutex>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QSGRectangleNode>
#include <components/playlist/qml_main_context.hpp>
#include "qt.hpp"
#include "main_interface.hpp"

class VideoRendererGL : public QObject
{
    Q_OBJECT
public:
    VideoRendererGL(MainInterface* p_mi,  QObject *parent = nullptr);
    ~VideoRendererGL();

public:
    QSharedPointer<QSGTexture> getDisplayTexture();

    static bool make_current_cb(void* data, bool current);
    static void* get_proc_address_cb(void* data, const char* procName);
    static void swap_cb(void* data);
    static bool setup_cb(void* data);
    static void cleanup_cb(void* data);
    static void resize_cb(void* data, unsigned width, unsigned height);

signals:
    void updated();
    void sizeChanged(QSize);

private:
    QMutex m_lock;
    QOpenGLContext* m_ctx = nullptr;
    QOffscreenSurface* m_surface = nullptr;

    MainInterface* m_mainInterface = nullptr;
    QQuickWindow* m_window = nullptr;
    QSize m_size;

    QSharedPointer<QSGTexture> m_textures[3] ;
    QOpenGLFramebufferObject* m_fbo[3];

    bool m_updated = false;
    int m_renderIdx = 0;
    int m_bufferIdx = 0;
    int m_displayIdx = 0;
};

class VideoSurfaceGL : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QmlMainContext* ctx READ getCtx WRITE setCtx NOTIFY ctxChanged)
    Q_PROPERTY(QSize sourceSize READ getSourceSize NOTIFY sourceSizeChanged)

public:
    VideoSurfaceGL(QQuickItem *parent = nullptr);
    ~VideoSurfaceGL();

public:
    QmlMainContext* getCtx();
    void setCtx(QmlMainContext* mainctx);

    QSize getSourceSize() const;

private slots:
    void onSizeChanged(QSize);

signals:
    void ctxChanged(QmlMainContext*);
    void sourceSizeChanged(QSize);

protected:
    bool m_sourceSizeChanged;
    QSize m_sourceSize;
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    QmlMainContext* m_mainCtx = nullptr;
    VideoRendererGL* m_renderer = nullptr;
    QSharedPointer<QSGTexture> m_displayTexture;
};



#endif // VIDEORENDERERGL_HPP

