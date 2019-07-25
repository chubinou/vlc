#include "qvoutwindowgl.hpp"
#include <QtQuick/QSGImageNode>
#include <QtQuick/QSGRectangleNode>
#include <QtQuick/QQuickWindow>
#include <QOpenGLFunctions>
#include <vlc_vout_window.h>
#include "main_interface.hpp"

QVoutWindowGL::QVoutWindowGL(MainInterface* p_mi, QObject* parent)
    : QVoutWindow(parent)
{
    assert(p_mi);
    m_ctx = new  QVoutWindowGLCtx();
    m_ctx->m_mainInterface = p_mi;
    m_ctx->obj = this;
    m_ctx->m_surfaceProvider.reset(new VideoSurfaceGL(this));
    m_ctx->m_refCount = 1;
    for (int i = 0; i < 3; i++)
    {
        m_ctx->m_fbo[i] = nullptr;
        m_ctx->m_textures[i] = nullptr;
    }
}

QVoutWindowGL::~QVoutWindowGL()
{
    assert(m_ctx);

    qWarning("QVoutWindowGL::~QVoutWindowGL");

    m_ctx->m_lock.lock();
    m_ctx->m_refCount--;
    if (m_ctx->m_refCount == 0) {
        m_ctx->m_lock.unlock();
        qWarning("destroy ctx");
        delete m_ctx;
    }
    else {
        m_ctx->obj = nullptr;
        m_ctx->m_lock.unlock();
    }

}

QSharedPointer<QSGTexture> QVoutWindowGL::getDisplayTexture()
{
    QMutexLocker lock(&m_ctx->m_lock);
    if (!m_ctx->m_hasTextures)
        return nullptr;
    if (m_ctx->m_updated)
    {
        qSwap(m_ctx->m_displayIdx, m_ctx->m_bufferIdx);
        m_ctx->m_updated = false;
    }
    m_ctx->m_needFlush = true;
    return m_ctx->m_textures[m_ctx->m_displayIdx];
}

//only one texture can be borrowed
void QVoutWindowGL::releaseTexture()
{
    QMutexLocker lock(&m_ctx->m_lock);
    m_ctx->m_needFlush = false;
    m_ctx->m_barrier.wakeOne();
}

bool QVoutWindowGL::make_current_cb(void* data, bool current)
{
    QVoutWindowGLCtx* ctx = static_cast<QVoutWindowGLCtx*>(data);
    QMutexLocker lock(&ctx->m_lock);
    if (!ctx->m_glctx || !ctx->m_surface)
    {
        return false;
    }

    if (current)
        return ctx->m_glctx->makeCurrent(ctx->m_surface);
    else
        ctx->m_glctx->doneCurrent();
    return true;
}

void*QVoutWindowGL::get_proc_address_cb(void* data, const char* procName)
{
    QVoutWindowGLCtx* ctx = static_cast<QVoutWindowGLCtx*>(data);
    return (void*)ctx->m_glctx->getProcAddress(procName);
}

void QVoutWindowGL::swap_cb(void* data)
{
    QVoutWindowGL* that = nullptr;
    QVoutWindowGLCtx* ctx = static_cast<QVoutWindowGLCtx*>(data);
    {
        QMutexLocker lock(&ctx->m_lock);
        qSwap(ctx->m_renderIdx, ctx->m_bufferIdx);
        ctx->m_updated = true;
        ctx->m_hasTextures = true;
        that  = ctx->obj;
    }
    ctx->m_fbo[ctx->m_renderIdx]->bind();
    if (that)
        emit that->updated();
}

bool QVoutWindowGL::setup_cb(void* data)
{
    QVoutWindowGLCtx* ctx = static_cast<QVoutWindowGLCtx*>(data);

    QMutexLocker lock(&ctx->m_lock);
    ctx->m_window = ctx->m_mainInterface->getRootQuickWindow();
    if (! ctx->m_window)
        return false;

    QOpenGLContext *current = ctx->m_window->openglContext();

    ctx->m_glctx = new QOpenGLContext();
    if (!ctx->m_glctx)
    {
        ctx->m_window = nullptr;
        return false;
    }
    QSurfaceFormat format = current->format();

    ctx->m_glctx->setFormat(format);
    ctx->m_glctx->setShareContext(current);
    ctx->m_glctx->create();
    if (!ctx->m_glctx->isValid())
    {
        msg_Err(ctx->obj->m_voutWindow, "unable to create openglContext");
        delete ctx->m_glctx;
        ctx->m_glctx = nullptr;
        ctx->m_window = nullptr;
        return false;
    }

    ctx->m_surface = new QOffscreenSurface();
    if (!ctx->m_surface)
    {
        msg_Err(ctx->obj->m_voutWindow, "unable to create offscreen surface");
        ctx->m_window = nullptr;
        delete ctx->m_glctx;
        ctx->m_glctx = nullptr;
        return false;
    }
    ctx->m_surface->setFormat(ctx->m_glctx->format());
    ctx->m_surface->create();
    if (!ctx->m_surface->isValid())
    {
        msg_Err(ctx->obj->m_voutWindow, "unable to create surface");
        delete ctx->m_surface;
        delete ctx->m_glctx;
        ctx->m_glctx = nullptr;
        ctx->m_window = nullptr;
        return false;
    }
    ctx->m_refCount++;

    return true;
}

void QVoutWindowGL::cleanup_cb(void* data)
{
    QVoutWindowGLCtx* ctx = static_cast<QVoutWindowGLCtx*>(data);

    qWarning("QVoutWindowGL::cleanup_cb");

    QMutexLocker lock(&ctx->m_lock);

    ctx->m_refCount--;
    if (ctx->m_refCount==0) {
        lock.unlock();
        qWarning("destroy ctx (from cleanup)");
        delete ctx;
        return;
    }


    ctx->m_hasTextures = false;
    if (ctx->m_needFlush)
    {
        if (ctx->obj)
            emit ctx->obj->updated();
        ctx->m_barrier.wait(&ctx->m_lock);
        ctx->m_needFlush = false;
    }

    for (int i =0; i < 3; i++)
    {
        if (ctx->m_fbo[i])
        {
            delete ctx->m_fbo[i];
            ctx->m_fbo[i] = nullptr;
        }
        if (ctx->m_textures[i])
        {
            ctx->m_textures[i] = nullptr;
        }
    }
    ctx->m_size = QSize();
    ctx->m_window = nullptr;

    if (ctx->m_surface) {
        delete ctx->m_surface;
        ctx->m_surface = nullptr;
    }
    if (ctx->m_glctx) {
        delete ctx->m_glctx;
        ctx->m_glctx = nullptr;
    }


}

void QVoutWindowGL::resize_cb(void* data, unsigned width, unsigned height)
{
    QVoutWindowGLCtx* ctx = static_cast<QVoutWindowGLCtx*>(data);

    QMutexLocker lock(&ctx->m_lock);
    QSize newsize(width, height);
    if (ctx->m_size != newsize)
    {
        ctx->m_size = newsize;
        for (int i =0; i < 3; i++)
        {
            if (ctx->m_fbo[i])
                delete ctx->m_fbo[i];
            ctx->m_fbo[i] = new QOpenGLFramebufferObject(newsize);
            ctx->m_textures[i] = QSharedPointer<QSGTexture>(ctx->m_window->createTextureFromId(ctx->m_fbo[i]->texture(), newsize));
            ctx->m_hasTextures = false;
        }
        if (ctx->obj)
            emit ctx->obj->sizeChanged(newsize);
    }
    ctx->m_fbo[ctx->m_renderIdx]->bind();
    //set the initial viewport
    ctx->m_glctx->functions()->glViewport(0, 0, width, height);
}

bool QVoutWindowGL::setupVoutWindow(vout_window_t* voutWindow)
{
    {
        QMutexLocker lock(&m_voutlock);
        if (m_voutWindow)
        {
            var_Destroy( m_voutWindow, "vout" );
            var_Destroy( m_voutWindow, "gles2" );
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
        return false;
    }

    var_Create( voutWindow, "vout", VLC_VAR_STRING );
    var_Create( voutWindow, "gles2", VLC_VAR_STRING );
    var_Create( voutWindow, "gl", VLC_VAR_STRING );

    if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGLES)
    {
        var_SetString ( voutWindow, "vout", "gles2" );
        var_SetString ( voutWindow, "gles2", "vgl");
    }
    else
    {
        var_SetString ( voutWindow, "vout", "gl" );
        var_SetString ( voutWindow, "gl", "vgl");
    }


    var_Create( voutWindow, "vout-cb-opaque", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-setup", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-cleanup", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-update-output", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-swap", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-make-current", VLC_VAR_ADDRESS );
    var_Create( voutWindow, "vout-cb-get-proc-address", VLC_VAR_ADDRESS );

    var_SetAddress( voutWindow, "vout-cb-opaque", m_ctx );
    var_SetAddress( voutWindow, "vout-cb-setup", (void*)&QVoutWindowGL::setup_cb );
    var_SetAddress( voutWindow, "vout-cb-cleanup", (void*)&QVoutWindowGL::cleanup_cb );
    var_SetAddress( voutWindow, "vout-cb-update-output", (void*)&QVoutWindowGL::resize_cb );
    var_SetAddress( voutWindow, "vout-cb-swap", (void*)&QVoutWindowGL::swap_cb );
    var_SetAddress( voutWindow, "vout-cb-make-current", (void*)&QVoutWindowGL::make_current_cb );
    var_SetAddress( voutWindow, "vout-cb-get-proc-address", (void*)&QVoutWindowGL::get_proc_address_cb );
    return true;
}

VideoSurfaceProvider*QVoutWindowGL::getVideoSurfaceProvider()
{
    return m_ctx->m_surfaceProvider.get();
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

VideoSurfaceGL::~VideoSurfaceGL()
{
    qWarning("VideoSurfaceGL::~VideoSurfaceGL");
    if (m_displayTexture)
    {
        m_renderer->releaseTexture();
        m_displayTexture = nullptr;
    }

}

QSGNode* VideoSurfaceGL::updatePaintNode(QQuickItem* item, QSGNode* oldNode, QQuickItem::UpdatePaintNodeData*)
{
    QSGRectangleNode* node = static_cast<QSGRectangleNode*>(oldNode);

    if (!node)
    {
        node = item->window()->createRectangleNode();
        node->setColor(Qt::black);
    }

    if (m_displayTexture) {
        m_displayTexture = nullptr;
        m_renderer->releaseTexture();
    }

    QSharedPointer<QSGTexture> newdisplayTexture = m_renderer->getDisplayTexture();
    if (!newdisplayTexture)
    {
        if (node->childCount() != 0) {
            node->removeAllChildNodes();;
        }
        node->setRect(item->boundingRect());
        return node;
    }
    m_displayTexture = newdisplayTexture;

    QSGSimpleTextureNode* texnode = nullptr;
    if (node->childCount() == 0)
    {
        texnode = new QSGSimpleTextureNode();
        texnode->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
        node->appendChildNode(texnode);
    }
    else
    {
        texnode = static_cast<QSGSimpleTextureNode*>(node->childAtIndex(0));
    }

    node->setRect(item->boundingRect());
    texnode->setTexture(m_displayTexture.data());
    texnode->setRect(item->boundingRect());
    texnode->markDirty(QSGNode::DirtyMaterial);
    return node;
}
