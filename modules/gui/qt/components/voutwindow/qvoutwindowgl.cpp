#include "qvoutwindowgl.hpp"
#include <QtQuick/QSGImageNode>
#include <QtQuick/QSGRectangleNode>
#include <QtQuick/QQuickWindow>
#include <QOpenGLFunctions>
#include <vlc_vout_window.h>
#include "main_interface.hpp"

QVoutWindowGL::QVoutWindowGL(MainInterface* p_mi, QObject* parent)
    : QVoutWindow(parent)
    , m_mainInterface(p_mi)
{
    assert(m_mainInterface);
    m_surfaceProvider.reset(new VideoSurfaceGL(this));
    for (int i = 0; i < 3; i++)
    {
        m_fbo[i] = nullptr;
        m_textures[i] = nullptr;
    }
}

QVoutWindowGL::~QVoutWindowGL()
{
    cleanup_cb(this);
}

QSharedPointer<QSGTexture> QVoutWindowGL::getDisplayTexture()
{
    QMutexLocker lock(&m_lock);
    if (!m_hasTextures)
        return nullptr;
    if (m_updated)
    {
        qSwap(m_displayIdx, m_bufferIdx);
        m_updated = false;
    }
    return m_textures[m_displayIdx];
}

bool QVoutWindowGL::make_current_cb(void* data, bool current)
{
    QVoutWindowGL* that = static_cast<QVoutWindowGL*>(data);
    QMutexLocker lock(&that->m_lock);
    if (!that->m_ctx || !that->m_surface)
    {
        return false;
    }

    if (current)
        return that->m_ctx->makeCurrent(that->m_surface);
    else
        that->m_ctx->doneCurrent();
    return true;
}

void*QVoutWindowGL::get_proc_address_cb(void* data, const char* procName)
{
    QVoutWindowGL* that = static_cast<QVoutWindowGL*>(data);
    return (void*)that->m_ctx->getProcAddress(procName);
}

void QVoutWindowGL::swap_cb(void* data)
{
    QVoutWindowGL* that = static_cast<QVoutWindowGL*>(data);
    {
        QMutexLocker lock(&that->m_lock);
        qSwap(that->m_renderIdx, that->m_bufferIdx);
        that->m_updated = true;
        that->m_hasTextures = true;
    }
    that->m_fbo[that->m_renderIdx]->bind();
    emit that->updated();
}

bool QVoutWindowGL::setup_cb(void* data)
{
    QVoutWindowGL* that = static_cast<QVoutWindowGL*>(data);

    QMutexLocker lock(&that->m_lock);
    that->m_window = that->m_mainInterface->getRootQuickWindow();
    if (! that->m_window)
        return false;

    QOpenGLContext *current = that->m_window->openglContext();

    that->m_ctx = new QOpenGLContext();
    if (!that->m_ctx)
    {
        that->m_window = nullptr;
        return false;
    }
    QSurfaceFormat format = current->format();

    that->m_ctx->setFormat(format);
    that->m_ctx->setShareContext(current);
    that->m_ctx->create();

    that->m_surface = new QOffscreenSurface();
    if (!that->m_surface)
    {
        that->m_window = nullptr;
        delete that->m_ctx;
        that->m_ctx = nullptr;
        return false;
    }
    that->m_surface->setFormat(that->m_ctx->format());
    that->m_surface->create();

    return true;
}

void QVoutWindowGL::cleanup_cb(void* data)
{
    QVoutWindowGL* that = static_cast<QVoutWindowGL*>(data);

    QMutexLocker lock(&that->m_lock);
    for (int i =0; i < 3; i++)
    {
        if (that->m_fbo[i])
        {
            delete that->m_fbo[i];
            that->m_fbo[i] = nullptr;
        }
        if (that->m_textures[i])
        {
            that->m_textures[i] = nullptr;
        }
    }
    that->m_size = QSize();
    that->m_window = nullptr;
    that->m_hasTextures = false;

    if (that->m_surface) {
        delete that->m_surface;
        that->m_surface = nullptr;
    }
    if (that->m_ctx) {
        delete that->m_ctx;
        that->m_ctx = nullptr;
    }
}

void QVoutWindowGL::resize_cb(void* data, unsigned width, unsigned height)
{
    QVoutWindowGL* that = static_cast<QVoutWindowGL*>(data);

    QMutexLocker lock(&that->m_lock);
    QSize newsize(width, height);
    if (that->m_size != newsize)
    {
        that->m_size = newsize;
        for (int i =0; i < 3; i++)
        {
            if (that->m_fbo[i])
                delete that->m_fbo[i];
            that->m_fbo[i] = new QOpenGLFramebufferObject(newsize);
            that->m_textures[i] = QSharedPointer<QSGTexture>(that->m_window->createTextureFromId(that->m_fbo[i]->texture(), newsize));
            that->m_hasTextures = false;
        }
        emit that->sizeChanged(newsize);
    }
    that->m_fbo[that->m_renderIdx]->bind();
    //set the initial viewport
    that->m_ctx->functions()->glViewport(0, 0, width, height);
}

void QVoutWindowGL::setupVoutWindow(vout_window_t* voutWindow)
{
    {
        QMutexLocker lock(&m_voutlock);
        if (m_voutWindow)
        {
            var_Destroy( m_voutWindow, "vout" );
            var_Destroy( m_voutWindow, "gl" );
            var_Destroy( m_voutWindow, "vout-cb-opaque" );
            var_Destroy( m_voutWindow, "vout-cb-setup" );
            var_Destroy( m_voutWindow, "vout-cb-cleanup" );
            var_Destroy( m_voutWindow, "vout-cb-update-output" );
            var_Destroy( m_voutWindow, "vout-cb-swap" );
            var_Destroy( m_voutWindow, "vout-cb-make-current" );
            var_Destroy( m_voutWindow, "vout-cb-get-proc-address" );
        }
    }
    QVoutWindow::setupVoutWindow(voutWindow);
    if (!voutWindow) {
        return;
    }

    var_Create( voutWindow, "vout", VLC_VAR_STRING );
    var_Create( voutWindow, "gl", VLC_VAR_STRING );

    var_SetString ( voutWindow, "vout", "gl" );
    var_SetString ( voutWindow, "gl", "vgl");

    var_Create( voutWindow, "vout-cb-opaque", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-setup", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-cleanup", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-update-output", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-swap", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-make-current", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-get-proc-address", VLC_VAR_ADDRESS );

    var_SetAddress( voutWindow, "vout-cb-opaque", this );
    var_SetAddress( voutWindow, "vout-cb-setup", (void*)&QVoutWindowGL::setup_cb );
    var_SetAddress( voutWindow, "vout-cb-cleanup", (void*)&QVoutWindowGL::cleanup_cb );
    var_SetAddress( voutWindow, "vout-cb-update-output", (void*)&QVoutWindowGL::resize_cb );
    var_SetAddress( voutWindow, "vout-cb-swap", (void*)&QVoutWindowGL::swap_cb );
    var_SetAddress( voutWindow, "vout-cb-make-current", (void*)&QVoutWindowGL::make_current_cb );
    var_SetAddress( voutWindow, "vout-cb-get-proc-address", (void*)&QVoutWindowGL::get_proc_address_cb );
}

VideoSurfaceProvider*QVoutWindowGL::getVideoSurfaceProvider()
{
    return m_surfaceProvider.get();
}

////////


VideoSurfaceGL::VideoSurfaceGL(QVoutWindowGL* renderer, QObject* parent)
    : VideoSurfaceProvider(parent)
    , m_renderer(renderer)
{
    connect(this, &VideoSurfaceGL::mouseMoved, m_renderer, &QVoutWindow::onMouseMoved);
    connect(this, &VideoSurfaceGL::mousePressed, m_renderer, &QVoutWindow::onMousePressed);
    connect(this, &VideoSurfaceGL::mouseDblClicked, m_renderer, &QVoutWindow::onMouseDoubleClick);
    connect(this, &VideoSurfaceGL::mouseReleased, m_renderer, &QVoutWindow::onMouseReleased);
    connect(this, &VideoSurfaceGL::mouseWheeled, m_renderer, &QVoutWindow::onMouseWheeled);
    connect(this, &VideoSurfaceGL::keyPressed, m_renderer, &QVoutWindow::onKeyPressed);

    connect(this, &VideoSurfaceGL::surfaceSizeChanged, m_renderer, &QVoutWindow::onSurfaceSizeChanged);

    connect(m_renderer, &QVoutWindowGL::updated, this, &VideoSurfaceGL::update, Qt::QueuedConnection);
    connect(m_renderer, &QVoutWindowGL::sizeChanged, this, &VideoSurfaceGL::sourceSizeChanged, Qt::QueuedConnection);
}

QSGNode* VideoSurfaceGL::updatePaintNode(QQuickItem* item, QSGNode* oldNode, QQuickItem::UpdatePaintNodeData*)
{
    QSGSimpleTextureNode* node = static_cast<QSGSimpleTextureNode*>(oldNode);

    QSharedPointer<QSGTexture> newdisplayTexture = m_renderer->getDisplayTexture();
    if (!newdisplayTexture)
    {
        if (node)
            node->setRect(item->boundingRect());
        return node;
    }
    m_displayTexture = newdisplayTexture;

    if (!node)
    {
        node = new QSGSimpleTextureNode();
        node->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    }

    node->setTexture(m_displayTexture.data());
    node->setRect(item->boundingRect());
    node->markDirty(QSGNode::DirtyMaterial);
    return node;
}
