//          
//          
//  File XModMandelpod.cpp   Resources and renderer for Mandelpod;
//  Garrett Vance 20170219
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//              
//          

#include "pch.h"
#include "XModMandelpod.h"
#include "..\Common\DirectXHelper.h"

using namespace HvyDX;
using namespace DirectX;
using namespace Windows::Foundation;




XModMandelpod::XModMandelpod(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
    mandelpod_loadingComplete(false),
    mandelpod_vertexCount(0),
    mandelpod_indexCount(0),
    m_deviceResources(deviceResources)
{
    CreateDeviceDependentResources();
}









#if 3 == 4
void XModMandelpod::Update(DX::StepTimer const& timer)
{
}
#endif 














void XModMandelpod::DrawIndexedPerMaterial(void)
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    uint32_t ib_index_offset = 0; 
    uint32_t loop_limit = (uint32_t)mandelpod_materialUsages->size(); 

    for (uint32_t idx_loop = 0; idx_loop < loop_limit; idx_loop++)
    {
        uint32_t materialOrdinal = (mandelpod_materialUsages->at(idx_loop).MaterialOrdinal);  //   % 7;

        ID3D11ShaderResourceView * arr_phong_srv[4] = 
        { 
            mandelpod_srv_mirror.Get(), 
            mandelpod_srv_black.Get(), 
            mandelpod_srv_normal.Get(), 
            mandelpod_srv_environment.Get() 
        };
            
        context->PSSetShaderResources(0, 4, arr_phong_srv);

        //   The number of index buffer entry usages will be 3x the count of triangle face usages. 

        uint32_t n_triangle_face_usages = mandelpod_materialUsages->at(idx_loop).UsageCount;  
        uint32_t n_ib_entry_usages = 3 * n_triangle_face_usages;

        context->DrawIndexed( n_ib_entry_usages,  ib_index_offset, 0 );

        ib_index_offset += n_ib_entry_usages; 
    }
}
//  Closes XModMandelpod::DrawIndexedPerMaterial();  









        
void XModMandelpod::Render(const DirectX::XMMATRIX& viewMat, const DirectX::XMMATRIX& projMat, int pSmall)
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    XMMATRIX mandelpod_worldMatrix; 
    HvyDX::VHG_MandelpodMatrixStruct* transformData = nullptr; 
    ID3D11Buffer* transformBuffer = nullptr; 
   
    if (pSmall > 0)
    {
        mandelpod_worldMatrix = XMLoadFloat4x4(&mandelpod_worldMatrix_3rdPerson_F4X4);
        transformData = &mandelpod_3rdPerson_transformData;
        transformBuffer = mandelpod_3rdPerson_transformBuffer.Get(); 
    }
    else
    {
        mandelpod_worldMatrix = XMLoadFloat4x4(&mandelpod_worldMatrix_1stPerson_F4X4);
        transformData = &mandelpod_1stPerson_transformData;
        transformBuffer = mandelpod_1stPerson_transformBuffer.Get(); 
    }

    XMMATRIX inverseTransposeWorld = XMMatrixInverse(nullptr, XMMatrixTranspose(mandelpod_worldMatrix));
    XMMATRIX inverseView = XMMatrixInverse(nullptr, viewMat);

    DirectX::XMStoreFloat4x4(
        &transformData->trxWVP, 
        XMMatrixTranspose(mandelpod_worldMatrix * viewMat * projMat)
    );

    DirectX::XMStoreFloat4x4(
        &transformData->trxWorld, 
        XMMatrixTranspose(mandelpod_worldMatrix)
    );

    DirectX::XMStoreFloat4x4(
        &transformData->trxInvTposeWorld, 
        XMMatrixTranspose(inverseTransposeWorld)
    );

    DirectX::XMStoreFloat4x4(
        &transformData->trxInverseView, 
        XMMatrixTranspose(inverseView)
    );

    context->UpdateSubresource1(transformBuffer, 0, NULL, transformData, 0, 0, 0);

    // 
    context->RSSetState(mandelpod_rasterizerState.Get());  //  TODO:  is this still necessary???

    UINT vb_stride = sizeof(WaveFrontReader<DWORD>::WFR_Vertex);
    UINT vb_offset = 0;
    context->IASetVertexBuffers(0, 1, mandelpod_vertexBuffer.GetAddressOf(), &vb_stride, &vb_offset);
    context->IASetIndexBuffer(mandelpod_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(mandelpod_waveFrontInputLayoutTNB.Get());  //  ghv added Tangent and Bitangent 20190204; 

    context->VSSetShader(mandelpod_vertexShader.Get(), nullptr, 0);
    ID3D11Buffer* constantBuffers[] = { transformBuffer }; 
    context->VSSetConstantBuffers1(0, 1, constantBuffers, nullptr, nullptr);

    context->PSSetShader(mandelpod_pixelShader.Get(), nullptr, 0);
    ID3D11SamplerState * arrSamplers[3] = { //   Must bind all three samplers for Phong Bump shading: 
        mandelpod_colorSampler.Get(),
        mandelpod_normalSampler.Get(),
        mandelpod_environmentSampler.Get()
    };
    context->PSSetSamplers(0, 3, arrSamplers);

    this->DrawIndexedPerMaterial();
}



















void XModMandelpod::CreateVertexBufferWavefrontOBJ(void)
{
    WaveFrontReader<DWORD>  stack_wfr;
    WaveFrontReader<DWORD>  *ptr_to_stack_wfr;
    ptr_to_stack_wfr = &stack_wfr;

#ifdef _DEBUG
    stack_wfr.Load(L"MeshFiles\\abSphere.wfobj", false);  //  Shiny mirrored disco ball;
#else
    stack_wfr.Load(L"MeshFiles\\frustumBrot.wfobj", false);  //  Frustum of Mandelbrot;
#endif

    UINT debug_plector_materials_card = 0; 
    debug_plector_materials_card = (UINT)stack_wfr.plector_materials.size(); 
    std::vector<WaveFrontReader<DWORD>::CWS_Material>       ptr_plector_materials = ptr_to_stack_wfr->plector_materials; 
    std::vector<uint32_t>                               ptr_plector_attributes = ptr_to_stack_wfr->plector_attributes; 


    //      The count of distinct int32_t values occurring in std::vector "plector_attributes" 
    //      will be needed. 
    //      Assumption #1: 
    //      Number of distinct int32_t values inside plector_attributes = plector_materials.size(); 
    //      Assumption #2:  
    //      One of these distinct values will refer to the "default material" 
    //      which won't ever be utilized by a WaveFront obj file / mtl file system. 
    //      So there will be -1 + plector_materials.size() "active" materials....
    //       

    VHG_MaterialUsageStruct     tmp_mat_usage;
    mandelpod_materialUsages = new std::vector<VHG_MaterialUsageStruct>(); 

    //     Special Case:   synthesize a "fake" element to begin the std::vector. 
    //     This "fake" element won't ever be accessed, but will keep 
    //     cardinalities in line with element index values: 
    tmp_mat_usage.MaterialOrdinal = 0; 
    tmp_mat_usage.UsageCount = 0; 
    mandelpod_materialUsages->push_back(tmp_mat_usage); 

    //     Now get the important MaterialUsage values: 
    uint32_t    prior_material = ptr_plector_attributes.at(0);   
    UINT        usage_count = 0; 
    std::vector<uint32_t>::iterator  iter_attribs; 
    for (iter_attribs = ptr_plector_attributes.begin(); iter_attribs != ptr_plector_attributes.end(); iter_attribs++)
    {
        if (*iter_attribs == prior_material)
        {
            usage_count++;
        }
        else
        {
            tmp_mat_usage.MaterialOrdinal = prior_material;
            tmp_mat_usage.UsageCount = usage_count; 
            mandelpod_materialUsages->push_back(tmp_mat_usage); 
            prior_material = *iter_attribs;
            usage_count = 1; 
        }
    }
    //  Then add the final VHG_MaterialUsage_struct: 
    tmp_mat_usage.MaterialOrdinal = prior_material;
    tmp_mat_usage.UsageCount = usage_count; 
    mandelpod_materialUsages->push_back(tmp_mat_usage); 
    //  Done counting MaterialUsages. 

    //      
    //          Note on Kd, Ka, Ks, shininess, specular etc...
    //      
    //  If this WaveFront OBJ file has materials at all (it might not), 
    //  then can access shading data like this: 
    //           
    //  uint32_t    exampleShininess = stack_wfr.plector_materials.at(4).nShininess;
    //  bool        exampleSpecular = stack_wfr.plector_materials.at(4).bSpecular;
    //           

    //      
    //          Compute Vertex Tangents
    //      
    //  example:  XMFLOAT3 rawNormal = stack_wfr.plector_vertices.at(29).normal;
    //      

    std::vector<WaveFrontReader<DWORD>::WFR_Vertex> &   ref_vertices = stack_wfr.plector_vertices;
    std::vector<DWORD> const & ref_indices = stack_wfr.plector_indices;

    ComputeVertexTangents(ref_vertices, ref_indices); 

    //  Create the Vertex Buffer for the WaveFrontReader: 

    mandelpod_vertexCount = (unsigned int)stack_wfr.plector_vertices.size();

    D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
    vertexBufferData.pSysMem = &(stack_wfr.plector_vertices[0]);
    vertexBufferData.SysMemPitch = 0;
    vertexBufferData.SysMemSlicePitch = 0;

    size_t total_allocation_vb = stack_wfr.plector_vertices.size() * sizeof(WaveFrontReader<DWORD>::WFR_Vertex);

    CD3D11_BUFFER_DESC vertexBufferDesc((UINT)total_allocation_vb, D3D11_BIND_VERTEX_BUFFER);

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &vertexBufferDesc,
            &vertexBufferData,
            &mandelpod_vertexBuffer
        )
    );

    //  Create the Index Buffer for the WaveFrontReader: 

    mandelpod_indexCount = (unsigned int)stack_wfr.plector_indices.size();

    D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
    indexBufferData.pSysMem = &(stack_wfr.plector_indices[0]);
    indexBufferData.SysMemPitch = 0;
    indexBufferData.SysMemSlicePitch = 0;

    size_t total_allocation_ib = stack_wfr.plector_indices.size() * sizeof(DWORD);

    CD3D11_BUFFER_DESC indexBufferDesc((UINT)total_allocation_ib, D3D11_BIND_INDEX_BUFFER);

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &indexBufferDesc,
            &indexBufferData,
            &mandelpod_indexBuffer
        )
    );
}















void XModMandelpod::CreateColorSampler()
{
    //  A bug in rendering the mirror was fixed by changing from CLAMP to WRAP; 
    //  Must use WRAP for U, V and W for both the mirror sampler and the bump sampler. 
    //      

    D3D11_SAMPLER_DESC samplerstate_descr;
    ZeroMemory(&samplerstate_descr, sizeof(samplerstate_descr));
    samplerstate_descr.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerstate_descr.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;  //  use WRAP for mirror and bump texture!!!
    samplerstate_descr.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;  //  use WRAP for mirror and bump textures!!!
    samplerstate_descr.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;  //  use WRAP for mirror and bump textures!!! 
    samplerstate_descr.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerstate_descr.MinLOD = 0;
    samplerstate_descr.MaxLOD = D3D11_FLOAT32_MAX;

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateSamplerState(
            &samplerstate_descr,
            mandelpod_colorSampler.ReleaseAndGetAddressOf()
        )
    );

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateSamplerState(
            &samplerstate_descr,
            mandelpod_normalSampler.ReleaseAndGetAddressOf()
        )
    );
}










void XModMandelpod::CreateEnvironmentSampler()
{
    D3D11_SAMPLER_DESC samplerstate_descr;
    ZeroMemory(&samplerstate_descr, sizeof(samplerstate_descr));
    samplerstate_descr.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerstate_descr.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerstate_descr.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP; 
    samplerstate_descr.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP; 
    samplerstate_descr.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerstate_descr.MinLOD = 0;
    samplerstate_descr.MaxLOD = D3D11_FLOAT32_MAX;

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateSamplerState(
            &samplerstate_descr,
            mandelpod_environmentSampler.ReleaseAndGetAddressOf()
        )
    );
}












void XModMandelpod::CreateRasterizerState()
{
    D3D11_RASTERIZER_DESC   rasterizerstate_descr;
    ZeroMemory(&rasterizerstate_descr, sizeof(rasterizerstate_descr));
    rasterizerstate_descr.DepthBias = 0;
    rasterizerstate_descr.SlopeScaledDepthBias = 0.0f;
    rasterizerstate_descr.DepthBiasClamp = 0.0f;
    rasterizerstate_descr.DepthClipEnable = TRUE;
    rasterizerstate_descr.ScissorEnable = FALSE;
    rasterizerstate_descr.AntialiasedLineEnable = FALSE;

    //  Specific rasterizer state for mandelpod: 

    rasterizerstate_descr.FillMode = D3D11_FILL_SOLID;
    rasterizerstate_descr.FrontCounterClockwise = FALSE;
    rasterizerstate_descr.CullMode = D3D11_CULL_NONE;
    rasterizerstate_descr.MultisampleEnable = TRUE;

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateRasterizerState(
        &rasterizerstate_descr,
        mandelpod_rasterizerState.ReleaseAndGetAddressOf()
    ));
}









void XModMandelpod::CreateDeviceDependentResources()
{
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

        CD3D11_BUFFER_DESC constantBufferDesc(sizeof(VHG_MandelpodMatrixStruct) , D3D11_BIND_CONSTANT_BUFFER);

        static_assert(
            (sizeof(VHG_MandelpodMatrixStruct) % 16) == 0,
            "Constant Buffer struct must be 16-byte aligned"
            );

        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(
                &constantBufferDesc,
                nullptr,
                &mandelpod_1stPerson_transformBuffer
                )
            );

        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(
                &constantBufferDesc,
                nullptr,
                &mandelpod_3rdPerson_transformBuffer
                )
            );
    });



    auto createCubeTask = (createPSTask && createVSTask).then([this] () 
    {
        CreateVertexBufferWavefrontOBJ();
    });


    createCubeTask.then([this] () 
    {
        mandelpod_loadingComplete = true;
    });
}







void XModMandelpod::ReleaseDeviceDependentResources()
{
    mandelpod_loadingComplete = false;

    mandelpod_vertexShader.Reset();
    mandelpod_waveFrontInputLayoutTNB.Reset();
    mandelpod_pixelShader.Reset();
    mandelpod_1stPerson_transformBuffer.Reset();
    mandelpod_3rdPerson_transformBuffer.Reset();
    mandelpod_vertexBuffer.Reset();
    mandelpod_indexBuffer.Reset();
}


