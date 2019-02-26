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


QVoutWindowDirectComposition::QVoutWindowDirectComposition(MainInterface* p_mi, QObject* parent)
    : QVoutWindow(parent)
    , m_mainInterface(p_mi)
{
    //  //create the video native surface
    //  m_videoWindow = new QWidget();
    //  //m_videoWindow->setWindowFlag(Qt::ToolTip); //so it won't appears in window list
    //  //m_videoWindow->setAttribute(Qt::WA_TranslucentBackground); //set the window as layered
    //  m_videoWindow->setAttribute(Qt::WA_NativeWindow);
    //  m_videoHwnd =(HWND) m_videoWindow->winId(); //this force the creation of the native widget
    //
    //  BOOL dwma_true = TRUE;
    //  //window won't be rendered on screen
    //  DwmSetWindowAttribute(m_videoHwnd, DWMWA_CLOAK, &dwma_true, sizeof(dwma_true));
    //  m_videoWindow->show();

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
}



VideoSurfaceProvider*QVoutWindowDirectComposition::getVideoSurfaceProvider()
{
    return m_surfaceProvider;
}

void QVoutWindowDirectComposition::setupVoutWindow(vout_window_t* window)
{
    QVoutWindow::setupVoutWindow(window);
    //window->type = VOUT_WINDOW_TYPE_HWND;
    //window->handle.hwnd = m_videoHwnd;

    ////run on UI thread
    //QMetaObject::invokeMethod(m_mainInterface, [this]() {

        ComPtr<IDXGIDevice> dxgiDevice;
        m_d3d11Device.As(&dxgiDevice);

        // Create the DirectComposition device object.
        HR(DCompositionCreateDevice(dxgiDevice.Get(),
                                    __uuidof(IDCompositionDevice),
                                    reinterpret_cast<void**>(m_dcompDevice.GetAddressOf())), "create device" );

        HR(m_dcompDevice->CreateTargetForHwnd((HWND)m_mainInterface->effectiveWinId(), TRUE, &m_dcompTarget), "create target");

        HR(m_dcompDevice->CreateVisual(&m_rootVisual), "create root visual");
        HR(m_dcompTarget->SetRoot(m_rootVisual.Get()), "set root visual");

        /**** create swapchain  */
        DXGI_FORMAT output_format = DXGI_FORMAT_B8G8R8A8_UNORM; //DXGI_FORMAT_R16G16B16A16_FLOAT for HDR
        int width = 1024;
        int height = 768;

        ComPtr<IDXGIAdapter> dxgi_adapter;
        HR(dxgiDevice->GetAdapter(dxgi_adapter.GetAddressOf()));
        ComPtr<IDXGIFactory2> dxgi_factory;
        HR(dxgi_adapter->GetParent(IID_PPV_ARGS(dxgi_factory.GetAddressOf())));

        DXGI_SWAP_CHAIN_DESC1 desc = { 0 };
        desc.Width = width;
        desc.Height = height;
        desc.Format = output_format;
        desc.Stereo = FALSE;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.BufferCount = 2;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        desc.Flags = 0;

        HR(dxgi_factory->CreateSwapChainForComposition(
            m_d3d11Device.Get(), &desc, nullptr,
            m_videoSwapChain.GetAddressOf()));

        HR(m_dcompDevice->CreateVisual(&m_videoVisual), "create video visual");
        HR(m_videoVisual->SetContent(m_videoSwapChain.Get()), "set video content");
        HR(m_rootVisual->AddVisual(m_videoVisual.Get(), TRUE, NULL), "add video visual to root");

        msg_Info(window, "m_videoSwapChain %p", m_videoSwapChain.Get());

        DXGI_SWAP_CHAIN_DESC1 desc2 = { 0 };
        HR(m_videoSwapChain->GetDesc1(&desc2));

        msg_Info(window, "m_videoSwapChain getDesc OK");

        var_Create(window->obj.libvlc, "vout", VLC_VAR_STRING );
        var_Create(window->obj.libvlc, "winrt-d3dcontext", VLC_VAR_INTEGER );
        var_Create(window->obj.libvlc, "winrt-swapchain", VLC_VAR_INTEGER );

        var_SetString( window->obj.libvlc, "vout", "direct3d11" );
        var_SetInteger(window->obj.libvlc, "winrt-d3dcontext", (int64_t)m_d3d11Context.Get());
        var_SetInteger(window->obj.libvlc, "winrt-swapchain", (int64_t)m_videoSwapChain.Get());

        //create visual for video layer
        ///  HR(m_dcompDevice->CreateVisual(&m_videoVisual), "create video visual");
        ///  HR(m_dcompDevice->CreateSurfaceFromHwnd(m_videoHwnd, &m_videoSurface), "create video surface from hwnd");
        ///  HR(m_videoVisual->SetContent(m_videoSurface.Get()), "set video content");
        ///  HR(m_rootVisual->AddVisual(m_videoVisual.Get(), TRUE, NULL), "add video visual to root");

        BOOL dwma_true = TRUE;
        ////set ui windows out of screen
        HWND uiHwnd = (HWND)m_mainInterface->mediacenterView->effectiveWinId();
        //m_mainInterface->setWindowFlag(Qt::ToolTip);
        DwmSetWindowAttribute(uiHwnd, DWMWA_CLOAK, &dwma_true, sizeof(dwma_true));

        //create visual for ui layer
        HR(m_dcompDevice->CreateVisual(&m_uiVisual), "create ui visual");
        HR(m_dcompDevice->CreateSurfaceFromHwnd(uiHwnd, &m_uiSurface), "create ui surface from hwnd");
        HR(m_uiVisual->SetContent(m_uiSurface.Get()), "set video content");
        HR(m_rootVisual->AddVisual(m_uiVisual.Get(), TRUE, m_videoVisual.Get()), "add video visual to root");

        ////commit compisition
        HR(m_dcompDevice->Commit(), "commit");
    //}, Qt::QueuedConnection, nullptr);
}

bool QVoutWindowDirectComposition::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_mainInterface)
    {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
        case QEvent::HoverMove:
        case QEvent::Enter:
        case QEvent::Leave:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::GrabMouse:
        case QEvent::UngrabMouse:
        case QEvent::GrabKeyboard:
        case QEvent::UngrabKeyboard:
        case QEvent::FocusAboutToChange:
            return QApplication::sendEvent(m_mainInterface->mediacenterView, event);
            break;
        case QEvent::Resize:
        {
            QResizeEvent* resizeEvent = static_cast<QResizeEvent*>(event);
            m_mainInterface->mediacenterView->resize(resizeEvent->size().width(), resizeEvent->size().height());
            return false;
        }
        default:
            break;
        }
    }
    return false;
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
