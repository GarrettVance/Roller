//          
//          
//  File Sphybox.cpp   Spherical Skybox Utility
//  Garrett Vance 20170219
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//              
//          
//               

#include "pch.h"
#include "Sphybox.h"
#include "..\Common\DirectXHelper.h"

using namespace HvyDX;
using namespace DirectX;
using namespace Windows::Foundation;




Sphybox::Sphybox(const std::shared_ptr<DX::DeviceResources>& deviceResources) 
    : 
    m_deviceResources(deviceResources),
    sphybox_loadingComplete(false)
{
    sphybox_sphere = std::make_unique<HvyDX::TexCoSphere>(
        deviceResources,
        XMFLOAT3(0.f, 0.f, 0.f), 
        1000.f,
        24
    );


	CreateDeviceDependentResources();
}






void Sphybox::Render(const XMFLOAT3& eyePos, const XMMATRIX& viewMat, const XMMATRIX& projMat )
{
    XMMATRIX T = XMMatrixTranslation(eyePos.x, eyePos.y, eyePos.z);

    XMMATRIX ViewProj = viewMat * projMat;

    XMMATRIX tmpMVP = XMMatrixMultiply(T, ViewProj);

    XMStoreFloat4x4(
        &sphybox_MVP_ConbufData.ModelViewProjection,
        XMMatrixTranspose(
            tmpMVP
        )
    );

    auto context = m_deviceResources->GetD3DDeviceContext();

    context->UpdateSubresource1(sphybox_MVP_ConbufBuffer.Get(), 0, NULL, &sphybox_MVP_ConbufData, 0, 0, 0);


    context->OMSetDepthStencilState(ghv_DepthStencilState.Get(), 0);


    context->RSSetState(sphybox_rasterizerState.Get()); 
    UINT stride = sizeof(XMFLOAT3);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, sphybox_sphere->tcs_vertex_buffer.GetAddressOf(), &stride, &offset);


    // hazard: sizeof elements in the index buffer: I'm using 32-bit ints. 
    context->IASetIndexBuffer(sphybox_sphere->tcs_index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0); // R32_UINT for index buffer of uint32_t;

    context->IASetInputLayout(sphybox_inputLayout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


    context->VSSetShader(sphybox_vertexShader.Get(), nullptr, 0 );
    context->VSSetConstantBuffers1( 0, 1, sphybox_MVP_ConbufBuffer.GetAddressOf(), nullptr, nullptr );  // Slot zero;


    context->PSSetShader(sphybox_pixelShader.Get(), nullptr, 0 );
    context->PSSetShaderResources(0, 1, sphybox_cube_srv.GetAddressOf());  // Slot zero;
    context->PSSetSamplers(0, 1, sphybox_cube_sampler_state.GetAddressOf());  // Slot zero;


    context->DrawIndexed(sphybox_index_count, 0, 0);
}







void Sphybox::CreateDepthStencilState()
{
    D3D11_DEPTH_STENCIL_DESC gv_depth_stencil_descr = { 0 };

    // Depth test parameters: use "LESS_EQUAL"    //---------------------------------------------------------------
    gv_depth_stencil_descr.DepthEnable = true;    gv_depth_stencil_descr.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;    gv_depth_stencil_descr.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    // Stencil test parameters    //---------------------------------------------------------------
    gv_depth_stencil_descr.StencilEnable = false;    gv_depth_stencil_descr.StencilReadMask = 0xFF;    gv_depth_stencil_descr.StencilWriteMask = 0xFF;
    // Stencil operations if pixel is front-facing    //---------------------------------------------------------------
    gv_depth_stencil_descr.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;    gv_depth_stencil_descr.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;    gv_depth_stencil_descr.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;    gv_depth_stencil_descr.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    // Stencil operations if pixel is back-facing    //---------------------------------------------------------------
    gv_depth_stencil_descr.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;    gv_depth_stencil_descr.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;    gv_depth_stencil_descr.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;    gv_depth_stencil_descr.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    DX::ThrowIfFailed(        m_deviceResources->GetD3DDevice()->CreateDepthStencilState(            &gv_depth_stencil_descr,            ghv_DepthStencilState.GetAddressOf()        )    );}








void Sphybox::CreateCubeSamplerState()
{
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));

    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateSamplerState(
            &sampDesc,
            sphybox_cube_sampler_state.ReleaseAndGetAddressOf())
    );
}









void Sphybox::CreateRasterizerState(void)
{
    D3D11_RASTERIZER_DESC   rasterizer_description;
    ZeroMemory(&rasterizer_description, sizeof(rasterizer_description));

    rasterizer_description.MultisampleEnable = FALSE;

    rasterizer_description.FillMode = D3D11_FILL_SOLID; // SOLID;
    rasterizer_description.CullMode = D3D11_CULL_NONE;  // CULL_NONE; 

    rasterizer_description.FrontCounterClockwise = FALSE; // FALSE;

    DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateRasterizerState(
        &rasterizer_description,
        sphybox_rasterizerState.ReleaseAndGetAddressOf()
    ));
}










void Sphybox::CreateDeviceDependentResources()
{

    DirectX::CreateDDSTextureFromFile(
        m_deviceResources->GetD3DDevice(), 
        L"Assets\\2_dds_sky_cube_1024.dds",
        0, 
        &sphybox_cube_srv, 
        0, 
        nullptr
    ); 


    this->CreateDepthStencilState();

    this->CreateCubeSamplerState();

    this->CreateRasterizerState();


    // Load shaders asynchronously.

    auto loadVSTask = DX::ReadDataAsync(L"Sphybox_VertexShader.cso");
    auto loadPSTask = DX::ReadDataAsync(L"Sphybox_PixelShader.cso");


    auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) 
    {
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateVertexShader(
                &fileData[0],
                fileData.size(),
                nullptr,
                &sphybox_vertexShader
                )
            );

        static const D3D11_INPUT_ELEMENT_DESC sphyboxVertexDesc [] =
        {
            { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateInputLayout(
                sphyboxVertexDesc,
                ARRAYSIZE(sphyboxVertexDesc),
                &fileData[0],
                fileData.size(),
                &sphybox_inputLayout
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
                &sphybox_pixelShader
                )
            );

        CD3D11_BUFFER_DESC sphyboxMVPConbufDesc(sizeof(VHG_Sphybox_MVP_Struct) , D3D11_BIND_CONSTANT_BUFFER);

        static_assert(
            (sizeof(VHG_Sphybox_MVP_Struct) % 16) == 0,
            "Constant Buffer struct must be 16-byte aligned"
            );

        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(
                &sphyboxMVPConbufDesc,
                nullptr,
                &sphybox_MVP_ConbufBuffer
                )
            );
    });
    



    auto createCubeTask = (createPSTask && createVSTask).then([this] () 
    {
        this->sphybox_sphere->LoadVertexBuffer(
            &sphybox_vertex_count,
            &sphybox_index_count
        );
    });


    
    createCubeTask.then([this] () 
    {
        sphybox_loadingComplete = true;
    });
}



