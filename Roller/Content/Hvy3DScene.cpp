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
	m_loadingComplete(false),
	m_indexCount(0),
    e_UsingMSAA(true),
	m_deviceResources(deviceResources), 

    //      
    //      
    // 
    e_ViewMatrixFixed(true), 
    // 
    //      
    //      
    kmi_degreesPerSecond(45),  // use 25 for debug;
    kmi_pitch(0),  
    kmi_yaw(0),  
    //      
    //      
    kmi_option_mesh_rotating(true),  
    kmi_option_rotation_direction(-1.f), 
    kmi_option_mesh_x_axis_ortho(true) // IMPORTANT : set to "true";
    //      
    //      
    //      
{


    e_ChiralityZOffset = +36.f;   // Use positive 36 for Chirality Left-handed; 




    //  instantiate the ParallelTransportFrame prior to calling CreateDeviceDependentResources: 
    //  Well now it's even more important to instantiate the PTF early, 
    //  because the Hvy3DScene::CreateWindowSizeDependentResources method
    //  now calls ParallelTransportFrame::CreateWindowSizeDependentResources. 
    //  
    //  hazard...
    //  

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
	float fovAngleY = 70.0f * XM_PI / 180.0f;


	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}
    e_xmmatrix_projection_trx = XMMatrixPerspectiveFovLH( fovAngleY, aspectRatio, 0.1f, 1000.0f ); // Chirality Left-handed; 




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

    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencil;

    DX::ThrowIfFailed(device->CreateTexture2D(
        &depthStencilDesc,
        nullptr,
        depthStencil.GetAddressOf()
    ));


    //  ghv: interesting fact: don't need to save the underlying depthStencil Texture2D - 
    //  rather it just gets discarded after it serves to create the persistent DSV: 


    DX::ThrowIfFailed(device->CreateDepthStencilView(
        depthStencil.Get(),
        nullptr,
        e_msaaDepthStencilView.ReleaseAndGetAddressOf()
    ));

}














DirectX::XMMATRIX Hvy3DScene::DirectionCosineMatrix(
    DirectX::XMFLOAT3 p_Tangent, 
    DirectX::XMFLOAT3 p_Normal, 
    DirectX::XMFLOAT3 p_Binormal
)
{
    //  The Space Curve of the Lorenz Attractor has, at any instant, 
    //  a local frame with orthonormal basis 
    //  (curveTangentHat, curveNormalHat, curveBinormalHat); 

    XMVECTOR curveTangentHat = XMVectorNegate(  //  signum;
        XMVector3Normalize(
            XMVectorSet(p_Tangent.x, p_Tangent.y, p_Tangent.z, 0.f)
        )
    ); 



    XMVECTOR curveNormalHat = XMVector3Normalize(
        XMVectorSet(p_Normal.x, p_Normal.y, p_Normal.z, 0.f)
    );
    curveNormalHat = XMVectorNegate(curveNormalHat); 



    XMVECTOR curveBinormalHat = XMVector3Normalize(
        XMVectorSet(p_Binormal.x, p_Binormal.y, p_Binormal.z, 0.f)
    );


    //  World Space has an orthonormal basis given by 
    //  the following three unit vectors: 

    XMVECTOR worldXHat = XMVectorSet(1.f, 0.f, 0.f, 0.f); 
    XMVECTOR worldYHat = XMVectorSet(0.f, 1.f, 0.f, 0.f); 
    XMVECTOR worldZHat = XMVectorSet(0.f, 0.f, 1.f, 0.f); 

    //  
    //  The World-to-Curve Rotation Matrix: 
    // 

    DirectX::XMFLOAT3X3     rotW2C;  //  W2C = World-to-Curve Transformation;

    rotW2C._11 = XMVectorGetX(XMVector3Dot(curveTangentHat, worldXHat)); 
    rotW2C._12 = XMVectorGetX(XMVector3Dot(curveTangentHat, worldYHat)); 
    rotW2C._13 = XMVectorGetX(XMVector3Dot(curveTangentHat, worldZHat)); 

    rotW2C._21 = XMVectorGetX(XMVector3Dot(curveNormalHat, worldXHat)); 
    rotW2C._22 = XMVectorGetX(XMVector3Dot(curveNormalHat, worldYHat)); 
    rotW2C._23 = XMVectorGetX(XMVector3Dot(curveNormalHat, worldZHat)); 

    rotW2C._31 = XMVectorGetX(XMVector3Dot(curveBinormalHat, worldXHat)); 
    rotW2C._32 = XMVectorGetX(XMVector3Dot(curveBinormalHat, worldYHat)); 
    rotW2C._33 = XMVectorGetX(XMVector3Dot(curveBinormalHat, worldZHat)); 

    DirectX::XMMATRIX     rotateWorldToCurve = XMLoadFloat3x3(&rotW2C);  

    return rotateWorldToCurve;
}



















        
void Hvy3DScene::CalculateViewMatrix(
    DirectX::XMFLOAT3 const& p_Position, 
    DirectX::XMFLOAT3 const& p_Tangent, 
    DirectX::XMFLOAT3 const& p_Normal
)
{
    XMVECTOR cameraWorldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // The "up" direction in World Coordinates;

    XMVECTOR effectiveUp; 

    XMVECTOR cameraLookAt;  


    if (e_ViewMatrixFixed == true)
    {
        // cameraPosition = XMVectorSet(40.f, 2.f, 28.f, 1.0f); // Use 40.f, 2.f, 28.f, 1.f;

        // better: cameraPosition = XMVectorSet(40.f, 2.f, 58.f, 1.0f); // Use 40.f, 2.f, 58.f, 1.f;  // better!

        cameraPosition = XMVectorSet(40.f, 2.f, 60.f, 1.0f); // Use 40.f, 2.f, 98.f, 1.f;  

        cameraLookAt = XMVectorSet(20.f, 0.5f, 66.f, 1.0f);  // Use 0.f, 0.5f, 46.f, 1.f;

        effectiveUp = cameraWorldUp;
    }
    else
    {
        XMVECTOR translatedXMV = XMVectorSet(p_Position.x - 2.f, p_Position.y, p_Position.z + e_ChiralityZOffset, 1.0f);

        //  "forwards" is synonymous with tangent: 
        XMVECTOR unNormalizedtangentXMV = XMVectorSet(p_Tangent.x, p_Tangent.y, p_Tangent.z, 0.f);  // homogeneous w_component = 0; 
        XMVECTOR normalizedTangentXMV = XMVector3Normalize(unNormalizedtangentXMV);

        //  "backwards" can be obtained from "forwards": 
        XMVECTOR normalizedBackwardsXMV = XMVectorNegate(normalizedTangentXMV);



        //  Normal vector from space curve (hazard: it points inward rather than outward): 
        //  
        //  TODO:  use new XMVECTOR member transported_normal: 
        //    
        XMVECTOR unNormalizedNormalXMV = XMVectorSet(p_Normal.x, p_Normal.y, p_Normal.z, 0.f);  // homogeneous w_component = 0; 
        XMVECTOR normalizedNormalXMV = XMVector3Normalize(unNormalizedNormalXMV); 



        //    
        //  cameraNormalDistance (like vertical elevation wrt the moving frame): 
        //    
        //  Usual value: cameraNormalDistance = -1.8f; 
        //  Observed: changing to cameraNormalDistance = -2.4f makes flipping much more violent, 
        //  seemingly exacerbating some discontinuity in the rotations. 
        //  

        float cameraNormalDistance = +2.f; //  -1.5f;  // Use -1.2f  or -1.8f (negative signum);


        //  
        //  CameraLookAheadDistance between 2.f and 4.f; 
        //  

        float cameraLookAheadDistance = 4.f; // Use cameraLookAheadDistance in [2.f, 4.f];


        //  
        //  CameraTrailingDistance between 2.f and 3.f; 
        //  Larger values e.g. 5.f will crash the application. 
        //  

        float cameraTrailingDistance = 3.f; // Use cameraTrailingDistance in [2.f, 3.f];



        //      
        //  For an interesting visual effect, use an altered calculation for cameraPosition below. 
        //  Change from + XMVectorScale(normalizedNormalXMV, cameraNormalDistance)
        //  to + XMVectorScale(XMVectorSet(0.f, 1.f, 0.f, 0.f), cameraNormalDistance);
        //  This amounts to replacing normalizedNormalXMV with the constant World Up vector. 
        //      


        //  Using lerp factor 0.7f gives a crazy bumpy ride on the roller coaster.


        XMVECTOR lerpedQuasiUp = XMVectorLerp(
            XMVectorNegate(normalizedNormalXMV),  // the mathematically proper direction; 
            XMVectorSet(0.f, 1.f, 0.f, 0.f),  //  the fixed World Up direction; 
            0.1f
        ); 


        cameraPosition = translatedXMV + XMVectorScale(normalizedBackwardsXMV, cameraTrailingDistance)
            + XMVectorScale(lerpedQuasiUp, cameraNormalDistance);


        cameraLookAt = translatedXMV + XMVectorScale(normalizedTangentXMV, cameraLookAheadDistance);


        effectiveUp = XMVectorNegate(normalizedNormalXMV);  //  Up direction in the Parallel Transported Frame;
    }

    e_xmmatrix_view_trx = XMMatrixLookAtLH(cameraPosition, cameraLookAt, effectiveUp);  //  CHIRALITY Left-handed;
}












uint32_t GetNextIdx(uint32_t p_card, uint32_t p_current)
{
    uint32_t idxNext = 1 + p_current; 

    if (idxNext + 2 == p_card)
    {
        idxNext = 0;
    }

    return idxNext;
}















void Hvy3DScene::Update(DX::StepTimer const& timer)
{
    DirectX::Keyboard::State        kb = kmi_keyboard->GetState();

    if (kb.F5)
    {
        if (e_ViewMatrixFixed == true)
        {
            e_ViewMatrixFixed = false;
        }
    }

    if (kb.F6)
    {
        if (e_ViewMatrixFixed == false)
        {
            e_ViewMatrixFixed = true;
        }
    }


    if (kb.F7)
    {
        if (e_UsingMSAA == true)
        {
            e_UsingMSAA = false;
        }
    }

    if (kb.F8)
    {
        if (e_UsingMSAA == false)
        {
            e_UsingMSAA = true;
        }
    }



    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    static uint32_t  idxUpdateCall = 0;

    idxUpdateCall = (1 + idxUpdateCall) % 3;

    //  undo   static uint32_t idxSpaceCurveElt = 0; // if not impatient;

    static uint32_t idxSpaceCurveElt = 1400;  // impatience factor: 1100, 900, etc...;


    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


    XMFLOAT3 spaceCurvePos{ 0.f, 0.f, 0.f };
    XMFLOAT3 spaceCurveTangent{ 0.f, 0.f, 0.f };  // ghv: added 20190311; 
    XMFLOAT3 spaceCurveNormal{ 0.f, 0.f, 0.f };  // ghv: added 20190313; 
    XMFLOAT3 spaceCurveBinormal{ 0.f, 0.f, 0.f };  // ghv: added 20190315; 

    if (!m_PTF->SpaceCurveVector().empty())
    {
        uint32_t card = (uint32_t)(m_PTF->SpaceCurveVector().size());

        spaceCurvePos = this->m_PTF->SpaceCurveVector().at(idxSpaceCurveElt).axon_position_r;
        spaceCurveTangent = this->m_PTF->SpaceCurveVector().at(idxSpaceCurveElt).axon_tangent_drdt; 
        spaceCurveNormal = this->m_PTF->SpaceCurveVector().at(idxSpaceCurveElt).axon_normal; 
        spaceCurveBinormal = this->m_PTF->SpaceCurveVector().at(idxSpaceCurveElt).axon_binormal; 


        // alternative idea:  uint32_t futureIdx = GetNextIdx( card, GetNextIdx(card, idxSpaceCurveElt) ); 


        if (idxUpdateCall == 0)
        {
            idxSpaceCurveElt += 1;

            if (idxSpaceCurveElt + 2 == card)
            {
                idxSpaceCurveElt = 0;
            }
        }
    }

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


    //      
    //  World Transformation of the Space Pod (aka MandelPod)
    //          
    //  Apply scaling to the Space Pod prior to translation: 
    //          

    float s5 = 2.f; 
    if (e_ViewMatrixFixed == true)
    {
        s5 = 4.f;

        // TODO: remove : e_rasterizer_fill_mode = D3D11_FILL_SOLID;
    }
    else
    {
        s5 = 3.f;

        // TODO: remove: e_rasterizer_fill_mode = D3D11_FILL_WIREFRAME;
    }

#ifdef _DEBUG

    //  For _DEBUG builds, the MandelPod is replaced 
    //  by the shiny sphere, which requires more drastic scaling: 

    s5 /= 3.f;  

#endif

    XMMATRIX spacePodScaling = XMMatrixScaling(s5, s5, s5);  

#ifdef _DEBUG
    float accumulatedRadians = (float)timer.GetTotalSeconds() * XMConvertToRadians(45.f);  

    float properRadians = static_cast<float>(fmod(accumulatedRadians, DirectX::XM_2PI)); 

    spacePodScaling = XMMatrixScaling(s5, s5, s5) * XMMatrixRotationX(properRadians);  
#endif


    XMMATRIX spacePodXlat = XMMatrixTranslation(spaceCurvePos.x - 2.f, spaceCurvePos.y, spaceCurvePos.z + e_ChiralityZOffset);

    XMMATRIX spacePodRotation = DirectionCosineMatrix(spaceCurveTangent, spaceCurveNormal, spaceCurveBinormal); 

    e_spacePodWorldTransformation = spacePodScaling * spacePodRotation * spacePodXlat;


    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


    CalculateViewMatrix(spaceCurvePos, spaceCurveTangent, spaceCurveNormal);


    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


    //      
    //  World Transformation of the Lorenz Attractor: 
    //   
    //  Need to push the Lorenz Attractor deeper into the scene's background. 
    //  This is accomplished by using e_ChiralityZOffset. 
    //      

    XMMATRIX translation_mx = XMMatrixTranslation(-2.f, 0.f, e_ChiralityZOffset);  
    XMMATRIX lorenz_scaling = XMMatrixScaling(1.f, 1.f, 1.f);

    XMMATRIX lorenzAttractorWorldTransformation = lorenz_scaling * translation_mx;

    if (this->m_PTF->LoadingComplete())
    {
        this->m_PTF->Update(
            lorenzAttractorWorldTransformation,
            e_xmmatrix_view_trx,
            e_xmmatrix_projection_trx 
        );
    }

}
















void Hvy3DScene::DrawIndexedPerMaterial(void)
{
	auto context = m_deviceResources->GetD3DDeviceContext();


    uint32_t ib_index_offset = 0; 
    uint32_t loop_limit = (uint32_t)e_vect_material_usages->size(); 

    for (uint32_t idx_loop = 0; idx_loop < loop_limit; idx_loop++)   //  
    {
        uint32_t materialOrdinal = (e_vect_material_usages->at(idx_loop).MaterialOrdinal);  //   % 7;

        ID3D11ShaderResourceView * arr_phong_srv[4] = { e_srv_mirror.Get(), e_srv_black.Get(), e_srv_normal.Get(), e_srv_environment.Get() }; 
            
        context->PSSetShaderResources(0, 4, arr_phong_srv);


        //   The number of index buffer entry usages will be 3x the count of triangle face usages. 

        uint32_t n_triangle_face_usages = e_vect_material_usages->at(idx_loop).UsageCount;  
        uint32_t n_ib_entry_usages = 3 * n_triangle_face_usages;

        context->DrawIndexed( n_ib_entry_usages,  ib_index_offset, 0 );

        ib_index_offset += n_ib_entry_usages; 
    }
}
//  Closes Hvy3DScene::DrawIndexedPerMaterial();  







void Hvy3DScene::Render()
{
	if (!m_loadingComplete)
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

    context->RSSetState(e_rasterizer_state_mandelpod.Get());

    RenderMandelPod();

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //    
    //  Render the spherical skybox: 
    //    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    context->RSSetState(e_rasterizer_state_solid.Get());

    XMFLOAT3 cameraPosXF3; 
    XMStoreFloat3(&cameraPosXF3, cameraPosition); 

    this->e_sphybox->Render(cameraPosXF3, e_xmmatrix_view_trx, e_xmmatrix_projection_trx); 

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //    
    //  Render the Lorenz Attractor loft: 
    //    
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    if (e_ViewMatrixFixed == false)
    {
        context->RSSetState(e_rasterizer_state_wireframe.Get());
    }
    else
    {
        context->RSSetState(e_rasterizer_state_solid.Get());
    }
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
        // PIXBeginEvent(context, PIX_COLOR_DEFAULT, L"Resolve");
        Microsoft::WRL::ComPtr<ID3D11Texture2D1> backBuffer;
        DX::ThrowIfFailed(m_deviceResources->GetSwapChain()->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
        context->ResolveSubresource(backBuffer.Get(), 0, e_msaaRenderTarget.Get(), 0, DX::DeviceResources::c_backBufferFormat);
        // PIXEndEvent(context);

        //  
        // Set render target for UI which is typically rendered without MSAA.
        //     
	    ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
        // undo ? context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());
        context->OMSetRenderTargets(1, targets, nullptr); // added at 2247;
    }
    context->RSSetState(e_rasterizer_state_solid.Get());
}









void Hvy3DScene::RenderMandelPod()
{
	auto context = m_deviceResources->GetD3DDeviceContext();

	DirectX::XMStoreFloat4x4(
		&e_conbuf_Transform_data.trxWVP, 
		XMMatrixTranspose(e_spacePodWorldTransformation * e_xmmatrix_view_trx * e_xmmatrix_projection_trx) 
	);


	DirectX::XMStoreFloat4x4(
		&e_conbuf_Transform_data.trxWorld, 
		XMMatrixTranspose(e_spacePodWorldTransformation)
	);

    //  matrix for inverse transpose of World transformation: 

    XMMATRIX inverseTransposeWorld = XMMatrixInverse(nullptr, XMMatrixTranspose(e_spacePodWorldTransformation)); 

	DirectX::XMStoreFloat4x4(
		&e_conbuf_Transform_data.trxInvTposeWorld, 
		XMMatrixTranspose(inverseTransposeWorld)
	);

    //  matrix for inverse of View transformation: 

    XMMATRIX inverseView = XMMatrixInverse(nullptr, e_xmmatrix_view_trx); 

	DirectX::XMStoreFloat4x4(
		&e_conbuf_Transform_data.trxInverseView, 
		XMMatrixTranspose(inverseView)
	);

	context->UpdateSubresource1( e_conbuf_Transform_buffer.Get(), 0, NULL, &e_conbuf_Transform_data, 0, 0, 0 );



    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //          
    //  Render the spacePod: 
    //      
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

	UINT vb_stride = sizeof(WaveFrontReader<DWORD>::WFR_Vertex);
	UINT vb_offset = 0;
	context->IASetVertexBuffers( 0, 1, m_vertexBuffer.GetAddressOf(), &vb_stride, &vb_offset );
    context->IASetIndexBuffer( m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0 );
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(m_waveFrontInputLayoutTNB.Get());  //  ghv added Tangent and Bitangent 20190204; 
	context->VSSetShader( m_vertexShader.Get(), nullptr, 0 );
	context->VSSetConstantBuffers1( 0, 1, e_conbuf_Transform_buffer.GetAddressOf(), nullptr, nullptr );
	context->PSSetShader( m_pixelShader.Get(), nullptr, 0 );

    //   Must bind all three samplers for Phong Bump shading: 

    ID3D11SamplerState * arrSamplers[3] = { 
        e_colorSampler.Get(), 
        e_normalSampler.Get(), 
        e_environmentSampler.Get() 
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
    e_vect_material_usages = new std::vector<VHG_MaterialUsage_struct>(); 

    //     Special Case:   synthesize a "fake" element to begin the std::vector. 
    //     This "fake" element won't ever be accessed, but will keep 
    //     cardinalities in line with element index values: 
    tmp_mat_usage.MaterialOrdinal = 0; 
    tmp_mat_usage.UsageCount = 0; 
    e_vect_material_usages->push_back(tmp_mat_usage); 

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
            e_vect_material_usages->push_back(tmp_mat_usage); 
            prior_material = *iter_attribs;
            usage_count = 1; 
        }
    }
    //  Then add the final VHG_MaterialUsage_struct: 
    tmp_mat_usage.MaterialOrdinal = prior_material;
    tmp_mat_usage.UsageCount = usage_count; 
    e_vect_material_usages->push_back(tmp_mat_usage); 
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

    m_vertexCount = (unsigned int)stack_wfr.plector_vertices.size();

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
            &m_vertexBuffer
        )
    );

    //  Create the Index Buffer for the WaveFrontReader: 

    m_indexCount = (unsigned int)stack_wfr.plector_indices.size();

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
            &m_indexBuffer
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
        e_rasterizer_state_mandelpod.ReleaseAndGetAddressOf()
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
            e_colorSampler.ReleaseAndGetAddressOf()
        )
    );

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateSamplerState(
            &samplerstate_descr,
            e_normalSampler.ReleaseAndGetAddressOf()
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
            e_environmentSampler.ReleaseAndGetAddressOf()
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









    this->m_PTF->CreateDeviceDependentResources();

    this->e_sphybox->CreateDeviceDependentResources();

    CreateRasterizerState();

    Microsoft::WRL::ComPtr<ID3D11Resource>   temp_resource;

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(
            m_deviceResources->GetD3DDevice(),
            L"Assets\\2_dds_color_type_dawning.dds", 
            temp_resource.ReleaseAndGetAddressOf(),
            e_srv_mirror.GetAddressOf(),
            0,
            nullptr
        )
    );

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(
            m_deviceResources->GetD3DDevice(),
            L"Assets\\2_dds_color_black_dawn.dds", 
            temp_resource.ReleaseAndGetAddressOf(),
            e_srv_black.GetAddressOf(),
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
            e_srv_environment.GetAddressOf(),
            nullptr
        )
    );

    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(
            m_deviceResources->GetD3DDevice(),
            L"Assets\\2_dds_bump_type_dawning.dds",
            temp_resource.ReleaseAndGetAddressOf(),
            e_srv_normal.GetAddressOf(),
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
				&m_vertexShader
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
				&m_waveFrontInputLayoutTNB
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
				&m_pixelShader
				)
			);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(VHG_conbuf_MVPA_struct) , D3D11_BIND_CONSTANT_BUFFER);

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&e_conbuf_Transform_buffer
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
		m_loadingComplete = true;
	});

}











void Hvy3DScene::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_vertexShader.Reset();

	m_waveFrontInputLayoutTNB.Reset();

	m_pixelShader.Reset();


	e_conbuf_Transform_buffer.Reset();


	m_vertexBuffer.Reset();
	m_indexBuffer.Reset();
}
