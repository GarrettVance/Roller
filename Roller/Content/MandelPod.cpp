//          
//          
//  File MandelPod.cpp   Renderer and resources for mesh model; 
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



void Hvy3DScene::DrawIndexedPerMaterial(void)
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
//  Closes Hvy3DScene::DrawIndexedPerMaterial();  









void Hvy3DScene::RenderMandelPodViewportSmall()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    XMMATRIX mandelpod_worldMatrix_1stPerson_MAT = XMLoadFloat4x4(&mandelpod_worldMatrix_1stPerson_F4X4);
    XMMATRIX mandelpod_worldMatrix_3rdPerson_MAT = XMLoadFloat4x4(&mandelpod_worldMatrix_3rdPerson_F4X4);

    XMMATRIX viewMatrix_1stPerson_MAT = XMLoadFloat4x4(&viewMatrix_1stPerson_F4X4);
    XMMATRIX viewMatrix_3rdPerson_MAT = XMLoadFloat4x4(&viewMatrix_3rdPerson_F4X4);

    XMMATRIX mProj = XMLoadFloat4x4(&m_ProjectionMatrix);

    XMMATRIX inverseTransposeWorld_1stPerson = XMMatrixInverse(nullptr, XMMatrixTranspose(mandelpod_worldMatrix_1stPerson_MAT));
    XMMATRIX inverseTransposeWorld_3rdPerson = XMMatrixInverse(nullptr, XMMatrixTranspose(mandelpod_worldMatrix_3rdPerson_MAT));

    XMMATRIX inverseView_1stPerson = XMMatrixInverse(nullptr, viewMatrix_1stPerson_MAT);
    XMMATRIX inverseView_3rdPerson = XMMatrixInverse(nullptr, viewMatrix_3rdPerson_MAT);







    {
        DirectX::XMStoreFloat4x4(
            &mandelpod_3rdPerson_transformData.trxWVP,
            XMMatrixTranspose(mandelpod_worldMatrix_3rdPerson_MAT * viewMatrix_3rdPerson_MAT * mProj)
        );

        DirectX::XMStoreFloat4x4(
            &mandelpod_3rdPerson_transformData.trxWorld,
            XMMatrixTranspose(mandelpod_worldMatrix_3rdPerson_MAT)
        );

        DirectX::XMStoreFloat4x4(
            &mandelpod_3rdPerson_transformData.trxInvTposeWorld,
            XMMatrixTranspose(inverseTransposeWorld_3rdPerson)
        );

        DirectX::XMStoreFloat4x4(
            &mandelpod_3rdPerson_transformData.trxInverseView,
            XMMatrixTranspose(inverseView_3rdPerson)
        );

        context->UpdateSubresource1(mandelpod_3rdPerson_transformBuffer.Get(), 0, NULL, &mandelpod_3rdPerson_transformData, 0, 0, 0);

        UINT vb_stride = sizeof(WaveFrontReader<DWORD>::WFR_Vertex);
        UINT vb_offset = 0;
        context->IASetVertexBuffers(0, 1, mandelpod_vertexBuffer.GetAddressOf(), &vb_stride, &vb_offset);
        context->IASetIndexBuffer(mandelpod_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->IASetInputLayout(mandelpod_waveFrontInputLayoutTNB.Get());  //  ghv added Tangent and Bitangent 20190204; 
        context->VSSetShader(mandelpod_vertexShader.Get(), nullptr, 0);
        context->VSSetConstantBuffers1(0, 1, mandelpod_3rdPerson_transformBuffer.GetAddressOf(), nullptr, nullptr);
        context->PSSetShader(mandelpod_pixelShader.Get(), nullptr, 0);
        ID3D11SamplerState * arrSamplers[3] = { //   Must bind all three samplers for Phong Bump shading: 
            mandelpod_colorSampler.Get(),
            mandelpod_normalSampler.Get(),
            mandelpod_environmentSampler.Get()
        };
        context->PSSetSamplers(0, 3, arrSamplers);
        this->DrawIndexedPerMaterial();
    }
}



void Hvy3DScene::RenderMandelPodViewportLarge()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    XMMATRIX mandelpod_worldMatrix_1stPerson_MAT = XMLoadFloat4x4(&mandelpod_worldMatrix_1stPerson_F4X4);
    // XMMATRIX mandelpod_worldMatrix_3rdPerson_MAT = XMLoadFloat4x4(&mandelpod_worldMatrix_3rdPerson_F4X4);

    XMMATRIX viewMatrix_1stPerson_MAT = XMLoadFloat4x4(&viewMatrix_1stPerson_F4X4);
    // XMMATRIX viewMatrix_3rdPerson_MAT = XMLoadFloat4x4(&viewMatrix_3rdPerson_F4X4);

    XMMATRIX mProj = XMLoadFloat4x4(&m_ProjectionMatrix);

    XMMATRIX inverseTransposeWorld_1stPerson = XMMatrixInverse(nullptr, XMMatrixTranspose(mandelpod_worldMatrix_1stPerson_MAT));
    // XMMATRIX inverseTransposeWorld_3rdPerson = XMMatrixInverse(nullptr, XMMatrixTranspose(mandelpod_worldMatrix_3rdPerson_MAT));

    XMMATRIX inverseView_1stPerson = XMMatrixInverse(nullptr, viewMatrix_1stPerson_MAT);
    // XMMATRIX inverseView_3rdPerson = XMMatrixInverse(nullptr, viewMatrix_3rdPerson_MAT);

    {
        DirectX::XMStoreFloat4x4(
            &mandelpod_1stPerson_transformData.trxWVP,
            XMMatrixTranspose(mandelpod_worldMatrix_1stPerson_MAT * viewMatrix_1stPerson_MAT * mProj)
        );

        DirectX::XMStoreFloat4x4(
            &mandelpod_1stPerson_transformData.trxWorld,
            XMMatrixTranspose(mandelpod_worldMatrix_1stPerson_MAT)
        );

        DirectX::XMStoreFloat4x4(
            &mandelpod_1stPerson_transformData.trxInvTposeWorld,
            XMMatrixTranspose(inverseTransposeWorld_1stPerson)
        );

        DirectX::XMStoreFloat4x4(
            &mandelpod_1stPerson_transformData.trxInverseView,
            XMMatrixTranspose(inverseView_1stPerson)
        );

        context->UpdateSubresource1(mandelpod_1stPerson_transformBuffer.Get(), 0, NULL, &mandelpod_1stPerson_transformData, 0, 0, 0);

        UINT vb_stride = sizeof(WaveFrontReader<DWORD>::WFR_Vertex);
        UINT vb_offset = 0;
        context->IASetVertexBuffers(0, 1, mandelpod_vertexBuffer.GetAddressOf(), &vb_stride, &vb_offset);
        context->IASetIndexBuffer(mandelpod_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->IASetInputLayout(mandelpod_waveFrontInputLayoutTNB.Get());  //  ghv added Tangent and Bitangent 20190204; 
        context->VSSetShader(mandelpod_vertexShader.Get(), nullptr, 0);
        context->VSSetConstantBuffers1(0, 1, mandelpod_1stPerson_transformBuffer.GetAddressOf(), nullptr, nullptr);
        context->PSSetShader(mandelpod_pixelShader.Get(), nullptr, 0);
        ID3D11SamplerState * arrSamplers[3] = { //   Must bind all three samplers for Phong Bump shading: 
            mandelpod_colorSampler.Get(),
            mandelpod_normalSampler.Get(),
            mandelpod_environmentSampler.Get()
        };
        context->PSSetSamplers(0, 3, arrSamplers);
        this->DrawIndexedPerMaterial();
    }


}

















void Hvy3DScene::CreateVertexBufferWavefrontOBJ(void)
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

    VHG_MaterialUsage_struct     tmp_mat_usage;
    mandelpod_materialUsages = new std::vector<VHG_MaterialUsage_struct>(); 

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




void Hvy3DScene::CreateColorSampler()
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


void Hvy3DScene::CreateEnvironmentSampler()
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





