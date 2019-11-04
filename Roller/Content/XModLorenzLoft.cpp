//                      
//                      

#include "pch.h"
#include "XModLorenzLoft.h"
#include "..\Common\DirectXHelper.h"

#include <fstream>
#include <string>
#include <sstream>
#include <iterator>

using namespace HvyDX;
using namespace DirectX;
using namespace Windows::Foundation;



using Microsoft::WRL::ComPtr;









XModLorenzLoft::XModLorenzLoft(const std::shared_ptr<DX::DeviceResources>& deviceResources) 
    :
    m_deviceResources(deviceResources),  
    loft_loadingComplete(false)
{  
    //   
    //  Typical Lorenz Attractor data file has 60 thousand records. 
    //  

#ifdef _DEBUG
    this->loft_axon_arc_density = 4096;   // Use 4096 to speed launch of debug builds;
#else
    this->loft_axon_arc_density = 8192;
#endif

    //      Configure the tubular loft:
    //      
    //  tube_radius = 0.5 will cause terrible overlap of trajectories; 
    //  tube_radius = 0.1 is about the largest tolerable value; 
    //      

    // undo this->loft_tube_radius = 0.05f;      // Spoke length and tube radius > 0.01. 

    this->loft_tube_radius = 0.08f;      // Spoke length and tube radius > 0.01. 

    this->loft_tube_facets = 6;
}  

















/*
void LorenzLoft::Update(
    XMMATRIX        const&           p_ParallelTransportFrameWorldMatrix,
    XMMATRIX        const&           p_parentSceneViewMatrix,
    XMMATRIX        const&           p_parentSceneProjectionMatrix
)
{
    DirectX::XMStoreFloat4x4(
        &loft_WVP_Data.model,
        XMMatrixTranspose(
            p_ParallelTransportFrameWorldMatrix
        )
    );

    DirectX::XMStoreFloat4x4(
        &loft_WVP_Data.view,
        XMMatrixTranspose(
            p_parentSceneViewMatrix
        )
    );

    DirectX::XMStoreFloat4x4(
        &loft_WVP_Data.projection,
        XMMatrixTranspose(
            p_parentSceneProjectionMatrix
        )
    );
}
*/







void XModLorenzLoft::Render(const DirectX::XMMATRIX& viewMat, const DirectX::XMMATRIX& projMat, int pSmall)
{ 
    if (!loft_loadingComplete)
    {
        return;
    }

    auto context = m_deviceResources->GetD3DDeviceContext();

    HvyDX::VHG_LoftMVPStruct* transformData = nullptr; 
    ID3D11Buffer* transformBuffer = nullptr; 
    ID3D11PixelShader* usePixelShader = nullptr;
   
    if (pSmall > 0)
    {
        transformData = &loft_WVP_ViewportMinor_Data;
        transformBuffer = loft_WVP_ViewportMinor_Buffer.Get(); 
        usePixelShader = loft_minorPixelShader.Get();
    }
    else
    {
        transformData = &loft_WVP_ViewportMajor_Data;
        transformBuffer = loft_WVP_ViewportMajor_Buffer.Get(); 
        usePixelShader = loft_majorPixelShader.Get();
    }

    context->UpdateSubresource1(transformBuffer, 0, NULL, transformData, 0, 0, 0);

    UINT vertex_buffer_2_stride = sizeof(VHG_Vertex_PosTex);
    UINT vertex_buffer_2_offset = 0;
    context->IASetVertexBuffers(
        0,
        1,
        loft_vertex_buffer_2_buffer.GetAddressOf(),
        &vertex_buffer_2_stride,
        &vertex_buffer_2_offset
    );
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(loft_inputLayout.Get());

    context->VSSetShader(loft_vertexShader.Get(), nullptr, 0);
    ID3D11Buffer* constantBuffers[] = { transformBuffer }; 
    context->VSSetConstantBuffers1(0, 1, constantBuffers, nullptr, nullptr);

    context->PSSetShader(usePixelShader, nullptr, 0); // depends on pSmall flag
    context->PSSetShaderResources(0, 1, loft_texture_srv.GetAddressOf());
    context->PSSetSamplers(0, 1, loft_texture_sampler_state.GetAddressOf());

    context->Draw( loft_vertex_buffer_2_count, 0 );
}  

























void XModLorenzLoft::Create_Vertex_Buffer(
    std::vector<HvyDX::VHG_Vertex_PosTex>  *p_vect_vertices,
    ID3D11Buffer** p_buffer_object
)
{ 
    size_t   bytes_required_allocation =
        sizeof(VHG_Vertex_PosTex) * p_vect_vertices->size();


    const VHG_Vertex_PosTex   *const_data_ptr = &(*p_vect_vertices)[0];


    D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
    ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));


    vertexBufferData.pSysMem = const_data_ptr;
    vertexBufferData.SysMemPitch = 0;
    vertexBufferData.SysMemSlicePitch = 0;


    CD3D11_BUFFER_DESC vertexBufferDesc(
        (UINT)bytes_required_allocation,   // the total required allocation;  
        D3D11_BIND_VERTEX_BUFFER
    );

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &vertexBufferDesc,
            &vertexBufferData,
            p_buffer_object
        )
    );
}  
//  Closes LorenzLoft::Create_Vertex_Buffer; 











void XModLorenzLoft::Create_Input_Layout(const std::vector<byte>& p_byte_vector)
{
    static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,  0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32_UINT,        0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateInputLayout(
            vertexDesc,
            ARRAYSIZE(vertexDesc),
            &p_byte_vector[0], 
            p_byte_vector.size(), 
            &loft_inputLayout
        )
    );
}  







void XModLorenzLoft::CreateDeviceDependentResources()
{ 
    Microsoft::WRL::ComPtr<ID3D11Resource>   temp_resource;

    DX::ThrowIfFailed(
        CreateDDSTextureFromFileEx(
            m_deviceResources->GetD3DDevice(),
            L"Assets\\1_dds_track.dds",
            (size_t)0,   //  maxsize,
            D3D11_USAGE_DEFAULT,
            D3D11_BIND_SHADER_RESOURCE,
            0,
            0,  // miscFlags;
            false,
            temp_resource.ReleaseAndGetAddressOf(),
            loft_texture_srv.GetAddressOf(),
            nullptr
        )
    );

    //      
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
    //                     Create a SamplerState 
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
    //  

    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP; // undo: revert to wrap;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateSamplerState(
                &sampDesc,
                loft_texture_sampler_state.ReleaseAndGetAddressOf()
        )
    );
    
    //      
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
    //                Load shaders asynchronously 
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
    //  

    auto loadVSTask = DX::ReadDataAsync(L"LoftVertexShader.cso");
    auto loadPSMinorTask = DX::ReadDataAsync(L"LoftMinorPixelShader.cso");
    auto loadPSMajorTask = DX::ReadDataAsync(L"LoftMajorPixelShader.cso");


    auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData)
    {
            DX::ThrowIfFailed(
                m_deviceResources->GetD3DDevice()->CreateVertexShader(
                    &fileData[0],
                    fileData.size(),
                    nullptr,
                    &loft_vertexShader
                )
            );

            this->Create_Input_Layout(fileData);
    });




    auto createPSMinorTask = loadPSMinorTask.then([this](const std::vector<byte>& fileData)
    {
            DX::ThrowIfFailed(
                m_deviceResources->GetD3DDevice()->CreatePixelShader(
                    &fileData[0],
                    fileData.size(),
                    nullptr,
                    &loft_minorPixelShader )
            );

            CD3D11_BUFFER_DESC constantBufferDesc(
                sizeof(VHG_LoftMVPStruct),
                D3D11_BIND_CONSTANT_BUFFER
            );

            static_assert(
                (sizeof(VHG_LoftMVPStruct) % 16) == 0,
                "Constant Buffer struct must be 16-byte aligned"
                );

            DX::ThrowIfFailed(
                m_deviceResources->GetD3DDevice()->CreateBuffer(
                    &constantBufferDesc,
                    nullptr,
                    &loft_WVP_ViewportMinor_Buffer )
            );
    });



    auto createPSMajorTask = loadPSMajorTask.then([this](const std::vector<byte>& fileData)
    {
            DX::ThrowIfFailed(
                m_deviceResources->GetD3DDevice()->CreatePixelShader(
                    &fileData[0],
                    fileData.size(),
                    nullptr,
                    &loft_majorPixelShader )
            );

            CD3D11_BUFFER_DESC constantBufferDesc(
                sizeof(VHG_LoftMVPStruct),
                D3D11_BIND_CONSTANT_BUFFER
            );

            static_assert(
                (sizeof(VHG_LoftMVPStruct) % 16) == 0,
                "Constant Buffer struct must be 16-byte aligned"
                );

            DX::ThrowIfFailed(
                m_deviceResources->GetD3DDevice()->CreateBuffer(
                    &constantBufferDesc,
                    nullptr,
                    &loft_WVP_ViewportMajor_Buffer )
            );
    });


    auto createCubeTask = (createPSMajorTask && createPSMinorTask && createVSTask).then([this]()
    {
            this->HansonParallelTransportFrame();
    });


    createCubeTask.then([this]()
    {
            this->loft_loadingComplete = true;
    });
}  






void XModLorenzLoft::ReleaseDeviceDependentResources()
{
    loft_loadingComplete = false; 

    loft_WVP_ViewportMinor_Buffer.Reset(); 
    loft_WVP_ViewportMajor_Buffer.Reset(); 
}  


