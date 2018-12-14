#include "videorenderergl.hpp"
#include <QtQuick/QSGImageNode>
#include <QtQuick/QSGRectangleNode>
#include <QtQuick/QQuickWindow>
#include "main_interface.hpp"

VideoSurfaceGL::VideoSurfaceGL(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents);
}

VideoSurfaceGL::~VideoSurfaceGL()
{
    printf("VideoSurfaceGL::~VideoSurfaceGL");
}

QmlMainContext*VideoSurfaceGL::getCtx()
{
    return m_mainCtx;
}

void VideoSurfaceGL::setCtx(QmlMainContext* mainctx)
{
    m_mainCtx = mainctx;
    emit ctxChanged(mainctx);
}

QSize VideoSurfaceGL::getSourceSize() const
{
    return m_sourceSize;
}

void VideoSurfaceGL::onSizeChanged(QSize newSize)
{
    m_sourceSize = newSize;
    emit sourceSizeChanged(m_sourceSize);
}

QSGNode* VideoSurfaceGL::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData*)
{
    QSGSimpleTextureNode* node = static_cast<QSGSimpleTextureNode*>(oldNode);

    if (m_renderer == nullptr) {
        m_renderer = m_mainCtx->getMainInterface()->getVideoRendererGL();
        connect(m_renderer, &VideoRendererGL::updated, this, &VideoSurfaceGL::update, Qt::QueuedConnection);
        connect(m_renderer, &VideoRendererGL::sizeChanged, this, &VideoSurfaceGL::onSizeChanged, Qt::QueuedConnection);
        return nullptr;
    }

    if (!node)
    {
        node = new QSGSimpleTextureNode();
        node->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    }

    QSharedPointer<QSGTexture> newdisplayTexture = m_renderer->getDisplayTexture();
    m_displayTexture = newdisplayTexture;
    if (!newdisplayTexture)
    {
        printf("VideoSurfaceGL::updatePaintNode no display texture\n");
        return node;
    }
    else {
        node->setTexture(m_displayTexture.data());
        node->setRect(boundingRect());
        node->markDirty(QSGNode::DirtyMaterial);
    }
    return node;
}


/////////////////////////

VideoRendererGL::VideoRendererGL(MainInterface* p_mi, QObject* parent)
    : QObject(parent)
    , m_mainInterface(p_mi)
{
    assert(m_mainInterface);
    for (int i = 0; i < 3; i++)
    {
        m_fbo[i] = nullptr;
        m_textures[i] = nullptr;
    }
}

VideoRendererGL::~VideoRendererGL()
{
    printf("VideoRendererGL::~VideoRendererGL\n");
}

QSharedPointer<QSGTexture> VideoRendererGL::getDisplayTexture()
{
    QMutexLocker lock(&m_lock);
    if (m_updated)
    {
        qSwap(m_displayIdx, m_bufferIdx);
        m_updated = false;
    }
    return m_textures[m_displayIdx];
}

bool VideoRendererGL::make_current_cb(void* data, bool current)
{
    VideoRendererGL* that = static_cast<VideoRendererGL*>(data);
    if (!that->m_ctx || !that->m_surface)
    {
        printf("VideoSurfaceGL::make_current_cb invalid context\n");
        return false;
    }

    if (current)
        return that->m_ctx->makeCurrent(that->m_surface);
    else
        that->m_ctx->doneCurrent();
    return true;
}

void*VideoRendererGL::get_proc_address_cb(void* data, const char* procName)
{
    VideoRendererGL* that = static_cast<VideoRendererGL*>(data);
    return (void*)that->m_ctx->getProcAddress(procName);
}

void VideoRendererGL::swap_cb(void* data)
{
    VideoRendererGL* that = static_cast<VideoRendererGL*>(data);
    {
        QMutexLocker lock(&that->m_lock);
        qSwap(that->m_renderIdx, that->m_bufferIdx);
        that->m_updated = true;
    }
    that->m_fbo[that->m_renderIdx]->bind();
    emit that->updated();
}

bool VideoRendererGL::setup_cb(void* data)
{
    printf("VideoSurfaceGL::setup_cb\n");
    VideoRendererGL* that = static_cast<VideoRendererGL*>(data);

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

void VideoRendererGL::cleanup_cb(void* data)
{
    printf("VideoSurfaceGL::cleanup_cb\n");
    VideoRendererGL* that = static_cast<VideoRendererGL*>(data);

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
}

void VideoRendererGL::resize_cb(void* data, unsigned width, unsigned height)
{
    printf("VideoSurfaceGL::resize_cb %ux%u\n", width, height);
    VideoRendererGL* that = static_cast<VideoRendererGL*>(data);

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
        }
        emit that->sizeChanged(newsize);
    }
    that->m_fbo[that->m_renderIdx]->bind();
}
