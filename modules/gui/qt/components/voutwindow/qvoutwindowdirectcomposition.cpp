#include "qvoutwindowdirectcomposition.hpp"
#include <QtQuick/QSGRectangleNode>
#include <QWidget>
#include <main_interface.hpp>
#include <dxgi1_3.h>

using namespace Microsoft::WRL;

struct ComException
{
    HRESULT result;
    ComException(HRESULT const value) :
        result(value)
    {}
};

static void HR(HRESULT const result, const char* msg = "")
{
    if (S_OK != result)
    {
        qWarning() << msg << "Failed";
        throw ComException(result);
    }
    else
    {
        qWarning() << msg << "OK";
    }
}


QVoutWindowDirectComposition::QVoutWindowDirectComposition(MainInterface* p_mi)
    : QVoutWindow(p_mi)
{
    assert(p_mi);
    m_surfaceProvider = new VideoSurfaceProviderDirectComposition( this, this );

    p_mi->installEventFilter(this);

    ComPtr<IDXGIFactory2> dxgiFactory;

    HR(CreateDXGIFactory2(
                0,
                __uuidof( IDXGIFactory2 ),
                &dxgiFactory ));

    ComPtr<IDXGIAdapter> dxgiAdapter;
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_VIDEO_SUPPORT;

    int i_adapter = 0;
    while( m_d3d11Device.Get() == nullptr )
    {
        HRESULT hr = dxgiFactory->EnumAdapters( i_adapter++, &dxgiAdapter );
        if (FAILED(hr) )
        {
            if( creationFlags & D3D11_CREATE_DEVICE_VIDEO_SUPPORT )
            {
                /* try again without this flag */
                i_adapter = 0;
                creationFlags &= ~D3D11_CREATE_DEVICE_VIDEO_SUPPORT;
                continue; //Try again with the new flags
            }
            else
                break; /* no more flags to remove */

        }

        hr = D3D11CreateDevice(
               dxgiAdapter.Get(),    // Adapter
               D3D_DRIVER_TYPE_UNKNOWN,
               nullptr,    // Module
               creationFlags,
               nullptr, 0, // Highest available feature level
               D3D11_SDK_VERSION,
               &m_d3d11Device,
               nullptr,    // Actual feature level
               &m_d3d11Context);
        if( FAILED( hr ) )
            m_d3d11Device.Reset();
    }

    ComPtr<IDXGIDevice> dxgiDevice;
    m_d3d11Device.As(&dxgiDevice);

    // Create the DirectComposition device object.
    HR(DCompositionCreateDevice(dxgiDevice.Get(),
                                __uuidof(IDCompositionDevice),
                                reinterpret_cast<void**>(m_dcompDevice.GetAddressOf())), "create device" );

    HR(m_dcompDevice->CreateTargetForHwnd((HWND)m_mainInterface->winId(), TRUE, &m_dcompTarget), "create target");

    HR(m_dcompDevice->CreateVisual(&m_rootVisual), "create root visual");
    HR(m_dcompTarget->SetRoot(m_rootVisual.Get()), "set root visual");

    ///**** create swapchain  */
    //DXGI_FORMAT output_format = DXGI_FORMAT_B8G8R8A8_UNORM; //DXGI_FORMAT_R16G16B16A16_FLOAT for HDR
    //int width = 1024;
    //int height = 768;
    //
    //ComPtr<IDXGIAdapter> dxgi_adapter;
    //HR(dxgiDevice->GetAdapter(dxgi_adapter.GetAddressOf()));
    //ComPtr<IDXGIFactory2> dxgi_factory;
    //HR(dxgi_adapter->GetParent(IID_PPV_ARGS(dxgi_factory.GetAddressOf())));
    //
    //DXGI_SWAP_CHAIN_DESC1 desc = { 0 };
    //desc.Width = width;
    //desc.Height = height;
    //desc.Format = output_format;
    //desc.Stereo = FALSE;
    //desc.SampleDesc.Count = 1;
    //desc.SampleDesc.Quality = 0;
    //desc.BufferCount = 2;
    //desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    //desc.Scaling = DXGI_SCALING_STRETCH;
    //desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    //desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    //desc.Flags = 0;
    //
    //HR(dxgi_factory->CreateSwapChainForComposition(
    //    m_d3d11Device.Get(), &desc, nullptr,
    //    m_videoSwapChain.GetAddressOf()));
    //
    HR(m_dcompDevice->CreateVisual(&m_videoVisual), "create video visual");
    //HR(m_videoVisual->SetContent(m_videoSwapChain.Get()), "set video content");
}

VideoSurfaceProvider*QVoutWindowDirectComposition::getVideoSurfaceProvider()
{
    return m_surfaceProvider;
}

QQuickWidget*QVoutWindowDirectComposition::createQuickWindow()
{
    QQuickWidget* mainWindow = new QQuickWidget(m_mainInterface);
    mainWindow->setWindowFlag(Qt::ToolTip); //set window as tooltip so it won't appears in window list
    mainWindow->setAttribute(Qt::WA_TranslucentBackground);
    mainWindow->setClearColor(Qt::transparent);

    BOOL dwma_true = TRUE;
    ////set ui windows out of screen
    HWND uiHwnd = (HWND)mainWindow->winId();
    DwmSetWindowAttribute(uiHwnd, DWMWA_CLOAK, &dwma_true, sizeof(dwma_true));

    //create visual for ui layer
    HR(m_dcompDevice->CreateVisual(&m_uiVisual), "create ui visual");
    HR(m_dcompDevice->CreateSurfaceFromHwnd(uiHwnd, &m_uiSurface), "create ui surface from hwnd");
    HR(m_uiVisual->SetContent(m_uiSurface.Get()), "set video content");
    HR(m_rootVisual->AddVisual(m_uiVisual.Get(), TRUE, NULL), "add video visual to root");

    HR(m_dcompDevice->Commit(), "commit");

    return mainWindow;
}

void QVoutWindowDirectComposition::setupVoutWindow(vout_window_t* window)
{
    if (m_voutWindow)
    {
        libvlc_int_t* libvlc = vlc_object_instance(m_voutWindow);
        var_Destroy(libvlc, "vout");
        //var_Destroy(libvlc, "winrt-d3dcontext");
        //var_Destroy(libvlc, "winrt-swapchain");
        //var_Destroy(libvlc, "direct3d11-dcomposition-cb");
        //var_Destroy(libvlc, "direct3d11-dcomposition-ptr");

        HR(m_rootVisual->RemoveVisual(m_videoVisual.Get()), "remove video visual from root");
    }

    QVoutWindow::setupVoutWindow(window);

    if (window)
    {
        window->type = VOUT_WINDOW_TYPE_DIRECTCOMPOSITION;
        window->handle.dcompvisual = m_uiVisual.Get();

        libvlc_int_t* libvlc = vlc_object_instance(window);
        var_Create(libvlc, "vout", VLC_VAR_STRING );
        //var_Create(libvlc, "winrt-d3dcontext", VLC_VAR_INTEGER );
        //var_Create(libvlc, "winrt-swapchain", VLC_VAR_INTEGER );
        //var_Create(libvlc, "direct3d11-dcomposition-cb",VLC_VAR_INTEGER );
        //var_Create(libvlc, "direct3d11-dcomposition-ptr",VLC_VAR_INTEGER );

        var_SetString( libvlc, "vout", "direct3d11" );
        //var_SetInteger(libvlc, "direct3d11-dcomposition-cb", (int64_t)&QVoutWindowDirectComposition::dcomposition_vout_cb);
        //var_SetInteger(libvlc, "direct3d11-dcomposition-ptr", (int64_t)this);
        //var_SetInteger(libvlc, "winrt-d3dcontext", (int64_t)m_d3d11Context.Get());
        //var_SetInteger(libvlc, "winrt-swapchain", (int64_t)m_videoSwapChain.Get());

        //place it bellow UI visual
        HR(m_rootVisual->AddVisual(m_videoVisual.Get(), FALSE, m_uiVisual.Get()), "remove video visual from root");
    }
    HR(m_dcompDevice->Commit(), "commit");
}

bool QVoutWindowDirectComposition::eventFilter(QObject* obj, QEvent* event)
{
#define PRINT_EVENT(S) case S: { qWarning("got event %s obj is %p (%smi)", #S, obj, obj == m_mainInterface ? "" : "not " ); break; }
    switch (event->type()) {
        PRINT_EVENT(QEvent::MouseMove)
        PRINT_EVENT(QEvent::MouseButtonPress)
        PRINT_EVENT(QEvent::MouseButtonRelease)
        PRINT_EVENT(QEvent::MouseButtonDblClick)
        PRINT_EVENT(QEvent::Wheel)
        PRINT_EVENT(QEvent::KeyPress)
        PRINT_EVENT(QEvent::KeyRelease)
        PRINT_EVENT(QEvent::HoverEnter)
        PRINT_EVENT(QEvent::HoverLeave)
        PRINT_EVENT(QEvent::HoverMove)
        PRINT_EVENT(QEvent::Enter)
        PRINT_EVENT(QEvent::Leave)
        PRINT_EVENT(QEvent::FocusIn)
        PRINT_EVENT(QEvent::FocusOut)
        PRINT_EVENT(QEvent::FocusAboutToChange)
        PRINT_EVENT(QEvent::GrabMouse)
        PRINT_EVENT(QEvent::UngrabMouse)
        PRINT_EVENT(QEvent::GrabKeyboard)
        PRINT_EVENT(QEvent::UngrabKeyboard)
    }

    switch (event->type()) {
    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:

    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
    case QEvent::Enter:
    case QEvent::Leave:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
    case QEvent::FocusAboutToChange:

    case QEvent::GrabMouse:
    case QEvent::UngrabMouse:
    case QEvent::GrabKeyboard:
    case QEvent::UngrabKeyboard:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    {
        return QApplication::sendEvent(m_mainInterface->mediacenterView->quickWindow(), event);
    }
    case QEvent::Resize:
    {
        QResizeEvent* resizeEvent = static_cast<QResizeEvent*>(event);
        m_mainInterface->mediacenterView->resize(resizeEvent->size().width(), resizeEvent->size().height());
        return false;
    }
    default:
        break;
    }
    return QObject::eventFilter(obj, event);
}

void QVoutWindowDirectComposition::dcomposition_vout_cb(void* data, void* userData)
{
    qWarning("\n\n\ndcomposition_vout_cb\n\n\n\n");
    QVoutWindowDirectComposition* that = (QVoutWindowDirectComposition*)userData;
    IDXGISwapChain* swapChain = (IDXGISwapChain*)data;
    that->m_videoVisual->SetContent(swapChain);
    that->m_dcompDevice->Commit();
}

VideoSurfaceProviderDirectComposition::VideoSurfaceProviderDirectComposition(QVoutWindowDirectComposition* renderer, QObject* parent)
    : VideoSurfaceProvider( parent )
    , m_renderer(renderer)
{
    connect(this, &VideoSurfaceProviderDirectComposition::mouseMoved, m_renderer, &QVoutWindow::onMouseMoved, Qt::QueuedConnection);
    connect(this, &VideoSurfaceProviderDirectComposition::mousePressed, m_renderer, &QVoutWindow::onMousePressed, Qt::QueuedConnection);
    connect(this, &VideoSurfaceProviderDirectComposition::mouseDblClicked, m_renderer, &QVoutWindow::onMouseDoubleClick, Qt::QueuedConnection);
    connect(this, &VideoSurfaceProviderDirectComposition::mouseReleased, m_renderer, &QVoutWindow::onMouseReleased, Qt::QueuedConnection);
    connect(this, &VideoSurfaceProviderDirectComposition::mouseWheeled, m_renderer, &QVoutWindow::onMouseWheeled, Qt::QueuedConnection);
    connect(this, &VideoSurfaceProviderDirectComposition::keyPressed, m_renderer, &QVoutWindow::onKeyPressed, Qt::QueuedConnection);

    connect(this, &VideoSurfaceProviderDirectComposition::surfaceSizeChanged, m_renderer, &QVoutWindow::onSurfaceSizeChanged);

    //connect(m_renderer, &QVoutWindowDirectComposition::updated, this, &VideoSurfaceProviderDirectComposition::update, Qt::QueuedConnection);
}

QSGNode* VideoSurfaceProviderDirectComposition::updatePaintNode(QQuickItem* item, QSGNode* oldNode, QQuickItem::UpdatePaintNodeData*)
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
