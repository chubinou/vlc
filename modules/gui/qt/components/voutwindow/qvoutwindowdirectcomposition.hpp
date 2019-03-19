/*****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef VIDEORENDERERDIRECTCOMPOSTION_HPP
#define VIDEORENDERERDIRECTCOMPOSTION_HPP

#include "qvoutwindow.hpp"
#include "videosurface.hpp"
#include <windows.h>

#include <d3d11.h>
#include <dcomp.h> //Direct Composition
#include <dwmapi.h>
#include <wrl.h> //for ComPtr

class VideoSurfaceProviderDirectComposition;

class QVoutWindowDirectComposition : public QVoutWindow
{
    Q_OBJECT
public:
    QVoutWindowDirectComposition(MainInterface* p_mi);

    VideoSurfaceProvider* getVideoSurfaceProvider() override;

    QQuickWidget* createQuickWindow() override;

    virtual void setupVoutWindow(vout_window_t* window) override;

    bool eventFilter(QObject *obj, QEvent *event);

private:
    static void dcomposition_vout_cb( void* data, void* userData );

    VideoSurfaceProviderDirectComposition* m_surfaceProvider = nullptr;

    QWidget* m_mainWindow;
    HWND m_mainHwnd = nullptr;

    QWidget* m_videoWindow;
    HWND m_videoHwnd = nullptr;

    Microsoft::WRL::ComPtr<ID3D11Device> m_d3d11Device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3d11Context;

    Microsoft::WRL::ComPtr<IDCompositionDevice> m_dcompDevice;
    Microsoft::WRL::ComPtr<IDCompositionTarget> m_dcompTarget;

    Microsoft::WRL::ComPtr<IDCompositionVisual> m_rootVisual;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> m_videoSwapChain;
    Microsoft::WRL::ComPtr<IDCompositionVisual> m_videoVisual;
    Microsoft::WRL::ComPtr<IDCompositionSurface> m_videoSurface;

    Microsoft::WRL::ComPtr<IDCompositionVisual> m_uiVisual;
    Microsoft::WRL::ComPtr<IDCompositionSurface> m_uiSurface;
};


class VideoSurfaceProviderDirectComposition : public VideoSurfaceProvider
{
    Q_OBJECT
public:

    VideoSurfaceProviderDirectComposition(QVoutWindowDirectComposition* qvoutwindow, QObject* parent = nullptr);
    QSGNode* updatePaintNode(QQuickItem* item, QSGNode* oldNode, QQuickItem::UpdatePaintNodeData*) override;

    QVoutWindowDirectComposition* m_renderer = nullptr;
};


#endif // VIDEORENDERERDIRECTCOMPOSTION_HPP
