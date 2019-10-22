//          
//          
//  File Multisample_AntiAliasing.cpp   Resources and methods for MSAA;
//  Garrett Vance 20191022
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//              
//          

#include "pch.h"
#include "Hvy3DScene.h"
#include "..\Common\DirectXHelper.h"

using namespace HvyDX;
using namespace DirectX;
using namespace Windows::Foundation;




void Hvy3DScene::MSAA_CreateWindowSizeDepResources()
{
    // Determine the render target size in pixels: 

    Size outputSize = m_deviceResources->GetOutputSize();
    UINT wRTPixels = static_cast<UINT>(outputSize.Width);  
    UINT hRTPixels = static_cast<UINT>(outputSize.Height);  
    UINT backBufferWidth = std::max<UINT>(wRTPixels, 1);
    UINT backBufferHeight = std::max<UINT>(hRTPixels, 1);

    // Create an MSAA render target:

    CD3D11_TEXTURE2D_DESC renderTargetDesc(
        DX::DeviceResources::c_backBufferFormat,
        backBufferWidth,
        backBufferHeight,
        1, // The render target view has only one texture.
        1, // Use a single mipmap level.
        D3D11_BIND_RENDER_TARGET,
        D3D11_USAGE_DEFAULT,
        0,
        e_MSAASampleCount
    );

    auto device = m_deviceResources->GetD3DDevice();

    DX::ThrowIfFailed(device->CreateTexture2D(
        &renderTargetDesc,
        nullptr,
        e_msaaRenderTarget.ReleaseAndGetAddressOf()
    ));

    CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(D3D11_RTV_DIMENSION_TEXTURE2DMS, DX::DeviceResources::c_backBufferFormat);

    DX::ThrowIfFailed(device->CreateRenderTargetView(
        e_msaaRenderTarget.Get(),
        &renderTargetViewDesc,
        e_msaaRenderTargetView.ReleaseAndGetAddressOf()
    ));

    // Create an MSAA depth stencil view.

    CD3D11_TEXTURE2D_DESC depthStencilDesc(
        DX::DeviceResources::c_depthBufferFormat,
        backBufferWidth,
        backBufferHeight,
        1, // This depth stencil view has only one texture.
        1, // Use a single mipmap level.
        D3D11_BIND_DEPTH_STENCIL,
        D3D11_USAGE_DEFAULT,
        0,
        e_MSAASampleCount
    );

    Microsoft::WRL::ComPtr<ID3D11Texture2D> temp_MSAA_depthStencil;

    DX::ThrowIfFailed(device->CreateTexture2D(
        &depthStencilDesc,
        nullptr,
        temp_MSAA_depthStencil.GetAddressOf()
    ));

    //  interesting:  don't need to save the underlying depthStencil Texture2D - 
    //  rather it just gets discarded...

    DX::ThrowIfFailed(device->CreateDepthStencilView(
        temp_MSAA_depthStencil.Get(),
        nullptr,
        e_msaaDepthStencilView.ReleaseAndGetAddressOf()
    ));
}







void Hvy3DScene::MSAA_Render()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //    
    //  Set the MSAA render target on the pipeline: 
    //    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    if (e_UsingMSAA)
    {
        ID3D11RenderTargetView* rtvs[1] = { e_msaaRenderTargetView.Get() };
        context->OMSetRenderTargets(1, rtvs, e_msaaDepthStencilView.Get()); // Set both the RTV and the DSV; 

        context->ClearRenderTargetView(e_msaaRenderTargetView.Get(), DirectX::Colors::Beige); // Beige...
        context->ClearDepthStencilView(e_msaaDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    }

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //    
    //  Render to the large viewport: 
    //    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    ID3D11Device3 * devic = m_deviceResources->GetD3DDevice();

    XMMATRIX viewMatrix_1stPerson_MAT = XMLoadFloat4x4(&viewMatrix_1stPerson_F4X4);
    XMMATRIX mProj = XMLoadFloat4x4(&m_ProjectionMatrix);

    // context->RSSetState(e_rasterizer_state_solid.Get());

    context->RSSetViewports(1, &m_deviceResources->GetViewport_Large());

    this->e_sphybox->Render(m_vEye, viewMatrix_1stPerson_MAT, mProj, -1); 

    context->RSSetState(mandelpod_rasterizerState.Get());
    RenderMandelPodViewportLarge();
    context->RSSetState(e_rasterizer_state_solid.Get());

    this->m_PTF->RenderViewportLarge(); // Render the Lorenz Attractor loft;  

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //    
    //  Render to the small viewport: 
    //    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    context->RSSetViewports(1, &m_deviceResources->GetViewport_Small());

    XMMATRIX viewMatrix_3rdPerson_MAT = XMLoadFloat4x4(&viewMatrix_3rdPerson_F4X4);
    this->e_sphybox->Render(m_vEye, viewMatrix_3rdPerson_MAT, mProj, 1); 

    this->m_PTF->RenderViewportSmall(); // Render the Lorenz Attractor loft;  

    RenderMandelPodViewportSmall();

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //    
    //  Resolve the MSAA render target: 
    //    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    if (e_UsingMSAA)
    {
        // Get a handle to the swap chain back buffer: 
        Microsoft::WRL::ComPtr<ID3D11Texture2D1> backBuffer;
        DX::ThrowIfFailed(m_deviceResources->GetSwapChain()->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));

        //  Resolve: 
        context->ResolveSubresource(backBuffer.Get(), 0, e_msaaRenderTarget.Get(), 0, DX::DeviceResources::c_backBufferFormat);

        // Revert state by setting the the back buffer RTV on the pipeline (to render HUD text): 
        ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
        context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());
    }
    context->RSSetState(e_rasterizer_state_solid.Get());
}







void Hvy3DScene::MSAA_TestDeviceSupport()
{
    for (e_MSAASampleCount = DX::DeviceResources::c_targetSampleCount; e_MSAASampleCount > 1; e_MSAASampleCount--)
    {
        UINT levels = 0;
        if (FAILED(m_deviceResources->GetD3DDevice()->CheckMultisampleQualityLevels(DX::DeviceResources::c_backBufferFormat, e_MSAASampleCount, &levels)))
        {
            continue;
        }

        if (levels > 0)
        {
            break;
        }
    }

    if (e_MSAASampleCount < 2)
    {
        throw std::exception("MSAA not supported");
    }
}


