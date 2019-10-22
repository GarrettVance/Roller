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


    /*
    if (aspectRatio < 1.0f)
    {
        fovAngleY *= 2.0f;
    }
    */




    XMMATRIX projMat = XMMatrixPerspectiveFovLH( fovAngleY, aspectRatio, 0.01f, 100.0f ); // Chirality Left-handed; 
    XMStoreFloat4x4(&m_ProjectionMatrix, projMat);




    // Allocate all memory resources that change on a window SizeChanged event:

    // Determine the render target size in pixels: 
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

    // ghv: Create an MSAA depth stencil view.

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
    //  Render the mandelpod: 
    //    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    context->RSSetState(mandelpod_rasterizerState.Get());

    RenderMandelPod();

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //    
    //  Render the spherical skybox: 
    //    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    context->RSSetState(e_rasterizer_state_solid.Get());

    XMMATRIX mView = XMLoadFloat4x4(&m_mView);
    XMMATRIX mProj = XMLoadFloat4x4(&m_ProjectionMatrix);

    this->e_sphybox->Render(m_vEye, mView, mProj); 

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //    
    //  Render the Lorenz Attractor loft: 
    //    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    
    context->RSSetState(e_rasterizer_state_solid.Get());

    this->m_PTF->Render(); // Render the Lorenz Attractor loft;  

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









void Hvy3DScene::RenderMandelPod()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    XMMATRIX mView = XMLoadFloat4x4(&m_mView);
    XMMATRIX mProj = XMLoadFloat4x4(&m_ProjectionMatrix);
    XMMATRIX mandelpod_worldMatrix_MAT = XMLoadFloat4x4(&mandelpod_worldMatrix_F4X4);

    DirectX::XMStoreFloat4x4(
        &mandelpod_transformData.trxWVP, 
        XMMatrixTranspose(mandelpod_worldMatrix_MAT * mView * mProj) 
    );

    DirectX::XMStoreFloat4x4(
        &mandelpod_transformData.trxWorld, 
        XMMatrixTranspose(mandelpod_worldMatrix_MAT)
    );

    //  matrix for inverse transpose of World transformation: 

    XMMATRIX inverseTransposeWorld = XMMatrixInverse(nullptr, XMMatrixTranspose(mandelpod_worldMatrix_MAT)); 

    DirectX::XMStoreFloat4x4(
        &mandelpod_transformData.trxInvTposeWorld, 
        XMMatrixTranspose(inverseTransposeWorld)
    );

    //  matrix for inverse of View transformation: 

    XMMATRIX inverseView = XMMatrixInverse(nullptr, mView); 

    DirectX::XMStoreFloat4x4(
        &mandelpod_transformData.trxInverseView, 
        XMMatrixTranspose(inverseView)
    );

    context->UpdateSubresource1( mandelpod_transformBuffer.Get(), 0, NULL, &mandelpod_transformData, 0, 0, 0 );

    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //          
    //  Render the mandelpod: 
    //      
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    UINT vb_stride = sizeof(WaveFrontReader<DWORD>::WFR_Vertex);
    UINT vb_offset = 0;
    context->IASetVertexBuffers( 0, 1, mandelpod_vertexBuffer.GetAddressOf(), &vb_stride, &vb_offset );

    context->IASetIndexBuffer( mandelpod_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0 );

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->IASetInputLayout(mandelpod_waveFrontInputLayoutTNB.Get());  //  ghv added Tangent and Bitangent 20190204; 

    context->VSSetShader( mandelpod_vertexShader.Get(), nullptr, 0 );
    context->VSSetConstantBuffers1( 0, 1, mandelpod_transformBuffer.GetAddressOf(), nullptr, nullptr );



    context->PSSetShader(mandelpod_pixelShader.Get(), nullptr, 0 );

    //   Must bind all three samplers for Phong Bump shading: 

    ID3D11SamplerState * arrSamplers[3] = { 
        mandelpod_colorSampler.Get(), 
        mandelpod_normalSampler.Get(), 
        mandelpod_environmentSampler.Get() 
    }; 
    context->PSSetSamplers(0, 3, arrSamplers);

    this->DrawIndexedPerMaterial();   
}




void Hvy3DScene::ComputeVertexTangents(
    std::vector<WaveFrontReader<DWORD>::WFR_Vertex>  &   refVertices,  // can't be const&
    std::vector<DWORD> const & refIndices
)
{
    //      
    //  Lengyel, Eric. “Computing Tangent Space Basis Vectors for an Arbitrary Mesh”. 
    //  Terathon Software, 2001. 
    //  http://terathon.com/code/tangent.html
    //

    uint32_t countOfIndices = (uint32_t)refIndices.size();
    uint32_t countOfTriangles = countOfIndices / 3;
    uint32_t countOfVertices = (uint32_t)refVertices.size(); 

    DirectX::XMVECTOR        *tan1Alloc = new DirectX::XMVECTOR[2 * countOfVertices]; 
    DirectX::XMVECTOR        *tan2Ptr = tan1Alloc + countOfVertices;  // do math on raw pointers;
        
    float w_component = 0.f;

    for (uint32_t a = 0; a < countOfTriangles; a++)
    {
        uint32_t i1 = refIndices.at(0 + 3 * a); 
        uint32_t i2 = refIndices.at(1 + 3 * a); 
        uint32_t i3 = refIndices.at(2 + 3 * a); 

        XMFLOAT3 const & v1 = refVertices.at(i1).position; 
        XMFLOAT3 const & v2 = refVertices.at(i2).position; 
        XMFLOAT3 const & v3 = refVertices.at(i3).position; 

        XMFLOAT2 const & w1 = refVertices.at(i1).textureCoordinate;
        XMFLOAT2 const & w2 = refVertices.at(i2).textureCoordinate;
        XMFLOAT2 const & w3 = refVertices.at(i3).textureCoordinate;

        float x1 = v2.x - v1.x;
        float x2 = v3.x - v1.x;
        float y1 = v2.y - v1.y;
        float y2 = v3.y - v1.y;
        float z1 = v2.z - v1.z;
        float z2 = v3.z - v1.z;

        float s1 = w2.x - w1.x;
        float s2 = w3.x - w1.x;
        float t1 = w2.y - w1.y;
        float t2 = w3.y - w1.y;

        float r = 1.0f / (s1 * t2 - s2 * t1);

        DirectX::XMVECTOR sdir = XMVectorSet(
            (t2 * x1 - t1 * x2) * r, 
            (t2 * y1 - t1 * y2) * r, 
            (t2 * z1 - t1 * z2) * r, 
            w_component
        );  

        DirectX::XMVECTOR tdir = XMVectorSet(
            (s1 * x2 - s2 * x1) * r, 
            (s1 * y2 - s2 * y1) * r,
            (s1 * z2 - s2 * z1) * r, 
            w_component
        );

        tan1Alloc[i1] += sdir;
        tan1Alloc[i2] += sdir;
        tan1Alloc[i3] += sdir;

        tan2Ptr[i1] += tdir;
        tan2Ptr[i2] += tdir;
        tan2Ptr[i3] += tdir;

    }  // Closes "for" loop;


    for (uint32_t a = 0; a < countOfVertices; a++)
    {
        //  Retrieve the already-computed Vertex Normal provided by the WaveFront OBJ file: 

        DirectX::XMVECTOR const & WaveFrontNormal = XMVectorSet( 
            refVertices[a].normal.x, 
            refVertices[a].normal.y, 
            refVertices[a].normal.z, 
            w_component
        );  

        DirectX::XMVECTOR const & urTangent = tan1Alloc[a]; 

        // Gram-Schmidt orthogonalize to get the x, y and z components of the Vertex Tangent: 

        XMVECTOR dotXMV = XMVector3Dot(WaveFrontNormal, urTangent);
        XMFLOAT3 dotF3; 
        XMStoreFloat3(&dotF3, dotXMV); 
        float dotFloat = dotF3.x; 

        XMVECTOR tan3Abnormal = urTangent - XMVectorScale(WaveFrontNormal, dotFloat);
        XMVECTOR tan3Hat = XMVector3Normalize(tan3Abnormal); 

        //  Compute the chirality and store it in the w-component of the Vertex Tangent: 

        XMVECTOR crossXMV = XMVector3Cross(WaveFrontNormal, urTangent); 
        XMVECTOR dotCrossXMV = XMVector3Dot(crossXMV, tan2Ptr[a]); 

        XMFLOAT3 dotCrossF3;
        XMStoreFloat3(&dotCrossF3, dotCrossXMV); 
        float dotCrossFloat = dotCrossF3.x; 

        float w_chirality = (dotCrossFloat < 0.0f) ? -1.0f : +1.0f;

        XMVECTOR finallyTangentXMV = XMVectorSetW(tan3Hat, w_chirality); 

        XMVECTOR BitangentXMV = XMVectorScale(
            XMVector3Cross(WaveFrontNormal, finallyTangentXMV),
            w_chirality
        );

        //   And now convert back to XMFLOAT3: 

        XMFLOAT3 tmpTangent;
        XMStoreFloat3(&tmpTangent, finallyTangentXMV); 
        refVertices[a].tangent = tmpTangent; 

        XMFLOAT3 tmpBitangent;
        XMStoreFloat3(&tmpBitangent, BitangentXMV); 
        refVertices[a].bitangent = tmpBitangent;
    }  //  Closes "for" loop; 

    delete[] tan1Alloc;
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










void Hvy3DScene::CreateDeviceDependentResources()
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

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

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
