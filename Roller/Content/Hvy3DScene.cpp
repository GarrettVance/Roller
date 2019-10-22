//          
//          
//  File Hvy3DScene.cpp   Main scene renderer;
//  Garrett Vance 20170219
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




Hvy3DScene::Hvy3DScene(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
    mandelpod_loadingComplete(false),
    mandelpod_vertexCount(0),
    mandelpod_indexCount(0),
    e_UsingMSAA(true),
    m_deviceResources(deviceResources), 
    e_View3rdPerson(false)
{
    e_ZOffset = +36.f;   // TODO: remove;


    //  instantiate the ParallelTransportFrame prior to calling CreateDeviceDependentResources: 

    m_PTF = std::make_unique<HvyDX::ParallelTransportFrame>(deviceResources);

    //  instantiate the Sphybox prior to calling CreateDeviceDependentResources: 

    e_sphybox = std::make_unique<HvyDX::Sphybox>(deviceResources);
        

    kmi_keyboard = std::make_unique<DirectX::Keyboard>(); 
    kmi_keyboard->SetWindow(Windows::UI::Core::CoreWindow::GetForCurrentThread());
    kmi_mouse = std::make_unique<DirectX::Mouse>(); 
    kmi_mouse->SetWindow(Windows::UI::Core::CoreWindow::GetForCurrentThread());


    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}




void Hvy3DScene::CreateWindowSizeDependentResources()
{
    Size outputSize = m_deviceResources->GetOutputSize();
    float aspectRatio = outputSize.Width / outputSize.Height;

    float fovAngleY = 85.0f * XM_PI / 180.0f;

    XMMATRIX projMat = XMMatrixPerspectiveFovLH( fovAngleY, aspectRatio, 0.01f, 100.0f ); // Chirality Left-handed; 
    XMStoreFloat4x4(&m_ProjectionMatrix, projMat);


    MSAA_CreateWindowSizeDepResources();
}







void Hvy3DScene::Render()
{
    if (!mandelpod_loadingComplete)
    {
        return;
    }

    if (!this->m_PTF->LoadingComplete())
    {
        return;
    }

    if (!this->e_sphybox->LoadingComplete())
    {
        return;
    }


    MSAA_Render();
}








void Hvy3DScene::CreateRasterizerState()
{
    D3D11_RASTERIZER_DESC   rasterizerstate_descr;
    ZeroMemory(&rasterizerstate_descr, sizeof(rasterizerstate_descr));
    rasterizerstate_descr.DepthBias = 0;
    rasterizerstate_descr.SlopeScaledDepthBias = 0.0f;
    rasterizerstate_descr.DepthBiasClamp = 0.0f;
    rasterizerstate_descr.DepthClipEnable = TRUE;
    rasterizerstate_descr.ScissorEnable = FALSE;
    rasterizerstate_descr.MultisampleEnable = TRUE;
    rasterizerstate_descr.AntialiasedLineEnable = FALSE;


    //  Generic solid rasterizer state: 

    rasterizerstate_descr.FillMode = D3D11_FILL_SOLID;
    rasterizerstate_descr.FrontCounterClockwise = FALSE;
    rasterizerstate_descr.CullMode = D3D11_CULL_BACK;

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateRasterizerState(
        &rasterizerstate_descr,
        e_rasterizer_state_solid.ReleaseAndGetAddressOf()
    ));

    //  Specific wireframe rasterizer state for ptf tube loft: 

    rasterizerstate_descr.FillMode = D3D11_FILL_WIREFRAME;
    rasterizerstate_descr.FrontCounterClockwise = FALSE;
    rasterizerstate_descr.CullMode = D3D11_CULL_NONE;

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateRasterizerState(
        &rasterizerstate_descr,
        e_rasterizer_state_wireframe.ReleaseAndGetAddressOf()
    ));

    //  Specific solid rasterizer state for mandelpod with reversed chirality: 

    rasterizerstate_descr.FillMode = D3D11_FILL_SOLID;
    rasterizerstate_descr.FrontCounterClockwise = false;
    rasterizerstate_descr.CullMode = D3D11_CULL_NONE;
    rasterizerstate_descr.MultisampleEnable = FALSE;

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateRasterizerState(
        &rasterizerstate_descr,
        mandelpod_rasterizerState.ReleaseAndGetAddressOf()
    ));
}









void Hvy3DScene::CreateDeviceDependentResources()
{
    MSAA_TestDeviceSupport(); 

    this->m_PTF->CreateDeviceDependentResources();

    this->e_sphybox->CreateDeviceDependentResources();

    CreateRasterizerState();

    Microsoft::WRL::ComPtr<ID3D11Resource>   temp_resource;

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(
            m_deviceResources->GetD3DDevice(),
            L"Assets\\2_dds_color_type_dawning.dds", 
            temp_resource.ReleaseAndGetAddressOf(),
            mandelpod_srv_mirror.GetAddressOf(),
            0,
            nullptr
        )
    );

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(
            m_deviceResources->GetD3DDevice(),
            L"Assets\\2_dds_color_black_dawn.dds", 
            temp_resource.ReleaseAndGetAddressOf(),
            mandelpod_srv_black.GetAddressOf(),
            0,
            nullptr
        )
    );

    DX::ThrowIfFailed(
        CreateDDSTextureFromFileEx(
            m_deviceResources->GetD3DDevice(),
            L"Assets\\2_dds_default_reflection.dds",
            (size_t)0,   //  maxsize,
            D3D11_USAGE_DEFAULT, 
            D3D11_BIND_SHADER_RESOURCE, 
            0, 
            D3D11_RESOURCE_MISC_TEXTURECUBE,
            false, 
            temp_resource.ReleaseAndGetAddressOf(),
            mandelpod_srv_environment.GetAddressOf(),
            nullptr
        )
    );

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(
            m_deviceResources->GetD3DDevice(),
            L"Assets\\2_dds_bump_type_dawning.dds",
            temp_resource.ReleaseAndGetAddressOf(),
            mandelpod_srv_normal.GetAddressOf(),
            0,
            nullptr
        )
    );

    //      Sampler State Objects for Pixel Shader: 

    CreateColorSampler(); 

    CreateEnvironmentSampler(); 

    //      Load shaders asynchronously: 

    auto loadVSTask = DX::ReadDataAsync(L"PhongBumpVertexShader.cso");
    auto loadPSTask = DX::ReadDataAsync(L"PhongBumpPixelShader.cso");

    auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) 
    {
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateVertexShader(
                &fileData[0],
                fileData.size(),
                nullptr,
                &mandelpod_vertexShader
                )
            );

        static const D3D11_INPUT_ELEMENT_DESC vertexDesc [] =
        {
            { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,   0,                             0,   D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT,   0,   D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT,      0,   D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT",    0, DXGI_FORMAT_R32G32B32_FLOAT,   0,   D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BITANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT,   0,   D3D11_APPEND_ALIGNED_ELEMENT,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateInputLayout(
                vertexDesc,
                ARRAYSIZE(vertexDesc),
                &fileData[0],
                fileData.size(),
                &mandelpod_waveFrontInputLayoutTNB
                )
            );
    });


    auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) 
    {
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreatePixelShader(
                &fileData[0],
                fileData.size(),
                nullptr,
                &mandelpod_pixelShader
                )
            );

        CD3D11_BUFFER_DESC constantBufferDesc(sizeof(VHG_conbuf_MVPA_struct) , D3D11_BIND_CONSTANT_BUFFER);

        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(
                &constantBufferDesc,
                nullptr,
                &mandelpod_transformBuffer
                )
            );
    });


    auto createCubeTask = (createPSTask && createVSTask).then([this] () 
    {
#ifdef GHV_OPTION_LOAD_MESH_MODEL
        CreateVertexBufferWavefrontOBJ();
#endif
    });


    createCubeTask.then([this] () 
    {
        mandelpod_loadingComplete = true;
    });
}



void Hvy3DScene::ReleaseDeviceDependentResources()
{
    mandelpod_loadingComplete = false;

    mandelpod_vertexShader.Reset();
    mandelpod_waveFrontInputLayoutTNB.Reset();
    mandelpod_pixelShader.Reset();
    mandelpod_transformBuffer.Reset();
    mandelpod_vertexBuffer.Reset();
    mandelpod_indexBuffer.Reset();
}


