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
    e_UsingMSAA(true),
    m_deviceResources(deviceResources)
{
    // e_ZOffset = +36.f;   // TODO: remove;


    //  instantiate the ParallelTransportFrame prior to calling CreateDeviceDependentResources: 
    m_PTF = std::make_unique<HvyDX::ParallelTransportFrame>(deviceResources);

    //  instantiate the Mandelpod prior to calling CreateDeviceDependentResources: 
    m_Mandelpod = std::make_unique<HvyDX::XModMandelpod>(deviceResources);

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










void Hvy3DScene::Update(DX::StepTimer const& timer)
{
    if (!this->m_Mandelpod->LoadingComplete())
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


    DirectX::Keyboard::State        kb = kmi_keyboard->GetState();

    if (kb.F7 && (e_UsingMSAA == true))
    {
        e_UsingMSAA = false;
    }

    if (kb.F8 && (e_UsingMSAA == false))
    {
        e_UsingMSAA = true;
    }

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


    static double time_now = timer.GetTotalSeconds();
    static double time_prior = timer.GetTotalSeconds();

    time_now = timer.GetTotalSeconds(); 

    float fElapsedTime = (float)(time_now - time_prior);


    static uint32_t  idxUpdateCall = 0;
    idxUpdateCall++;
    static uint32_t idxSpaceCurveElt = 0;  // impatience factor: 1400;

    XMFLOAT3 spaceCurvePos{ 0.f, 0.f, 0.f };
    XMFLOAT3 spaceCurveTangent{ 0.f, 0.f, 0.f };  // ghv: added 20190311; 
    XMFLOAT3 spaceCurveNormal{ 0.f, 0.f, 0.f };  // ghv: added 20190313; 
    XMFLOAT3 spaceCurveBinormal{ 0.f, 0.f, 0.f };  // ghv: added 20190315; 

    if (!m_PTF->ptf_axons.empty())
    {
        uint32_t card = (uint32_t)(m_PTF->ptf_axons.size());

        spaceCurvePos = this->m_PTF->ptf_axons.at(idxSpaceCurveElt).axon_position_r;
        spaceCurveTangent = this->m_PTF->ptf_axons.at(idxSpaceCurveElt).axon_tangent_drdt; 
        spaceCurveNormal = this->m_PTF->ptf_axons.at(idxSpaceCurveElt).axon_normal; 
        spaceCurveBinormal = this->m_PTF->ptf_axons.at(idxSpaceCurveElt).axon_binormal; 

        if (fElapsedTime > 0.03f)
        {
            time_prior = time_now;
            idxSpaceCurveElt += 1;
            if (idxSpaceCurveElt + 2 == card) idxSpaceCurveElt = 0; // wrap-around;
        }
    }

    // Calculate the correct rotation matrix to align the mandelPod: 

    XMMATRIX mCameraRot = DirectionCosineMatrix(spaceCurveTangent, spaceCurveNormal, spaceCurveBinormal); 

    CalculateViewMatrix_Following( spaceCurvePos, spaceCurveTangent, spaceCurveNormal, mCameraRot ); 















    //      
    //  World Transformation of the Space Pod (aka MandelPod)
    //          
#ifdef _DEBUG
    float accumulatedRadians = (float)timer.GetTotalSeconds() * XMConvertToRadians(45.f);  
    float properRadians = static_cast<float>(fmod(accumulatedRadians, DirectX::XM_2PI)); 
    XMMATRIX spacePodSpin = XMMatrixRotationX(properRadians);  
#else
    XMMATRIX spacePodSpin = XMMatrixIdentity();
#endif

    XMMATRIX spacePodRotation = DirectionCosineMatrix(spaceCurveTangent, spaceCurveNormal, spaceCurveBinormal); 


    XMMATRIX spacePodXlat_1stPerson = XMMatrixTranslation(spaceCurvePos.x - 2.f, spaceCurvePos.y, spaceCurvePos.z + e_ZOffset);


#ifdef _DEBUG
    XMMATRIX spacePodScaling1stPerson = XMMatrixScaling(0.7f, 0.7f, 0.7f);
#else
    XMMATRIX spacePodScaling1stPerson = XMMatrixScaling(4.f, 4.f, 4.f);
#endif

    XMMATRIX mandelpod_worldMatrix_1stPerson_MAT = spacePodScaling1stPerson * spacePodSpin * spacePodRotation * spacePodXlat_1stPerson;
    XMFLOAT4X4* world1stPerson_F4X4 = &this->m_Mandelpod->mandelpod_worldMatrix_1stPerson_F4X4; 
    XMStoreFloat4x4(world1stPerson_F4X4, mandelpod_worldMatrix_1stPerson_MAT);



#ifdef _DEBUG
    XMMATRIX spacePodScaling3rdPerson = XMMatrixScaling(1.6f, 1.6f, 1.6f);
#else
    XMMATRIX spacePodScaling3rdPerson = XMMatrixScaling(5.f, 5.f, 5.f); 
#endif


    const float xlat_x_3rdPerson = 0.f;  //  0.f; 
    const float xlat_z_3rdPerson = -10.f;  //  0.f;
    XMMATRIX spacePodXlat_3rdPerson = XMMatrixTranslation(spaceCurvePos.x + xlat_x_3rdPerson, spaceCurvePos.y, spaceCurvePos.z + xlat_z_3rdPerson);

    XMMATRIX attitude_3rdPerson = XMMatrixRotationY(-XM_PI / 4);
    attitude_3rdPerson = XMMatrixIdentity();

    XMMATRIX mandelpod_worldMatrix_3rdPerson_MAT = spacePodScaling3rdPerson * spacePodSpin * spacePodRotation * spacePodXlat_3rdPerson * attitude_3rdPerson;
    XMFLOAT4X4* world3rdPerson_F4X4 = &this->m_Mandelpod->mandelpod_worldMatrix_3rdPerson_F4X4; 
    XMStoreFloat4x4(world3rdPerson_F4X4, mandelpod_worldMatrix_3rdPerson_MAT);
















    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //  World Transformation of the Lorenz Attractor: 
    //   
    //  Need to push the Lorenz Attractor deeper into the scene's background. 
    //  This is accomplished by using e_ZOffset. 
    //      

    XMMATRIX loft_1stPerson_Translation = XMMatrixTranslation(-2.f, 0.f, e_ZOffset);  
    XMMATRIX loft_1stPerson_WorldTransformation = loft_1stPerson_Translation;


    XMMATRIX loft_3rdPerson_Translation = XMMatrixTranslation(xlat_x_3rdPerson, 0.f, xlat_z_3rdPerson);  
    XMMATRIX loft_3rdPerson_WorldTransformation = loft_3rdPerson_Translation * attitude_3rdPerson; 

    XMMATRIX viewMatrix_1stPerson_MAT = XMLoadFloat4x4(&viewMatrix_1stPerson_F4X4);
    XMMATRIX viewMatrix_3rdPerson_MAT = XMLoadFloat4x4(&viewMatrix_3rdPerson_F4X4);

    XMMATRIX projectionMatrix_MAT = XMLoadFloat4x4(&m_ProjectionMatrix);


    DirectX::XMStoreFloat4x4(
        &m_PTF->ptf_WVP_ViewportSmall_Data.model,
        XMMatrixTranspose(
            loft_3rdPerson_WorldTransformation  // 3rdPerson is rendered in small viewport;
        )
    );
    DirectX::XMStoreFloat4x4(
        &m_PTF->ptf_WVP_ViewportSmall_Data.view,
        XMMatrixTranspose(
            viewMatrix_3rdPerson_MAT
        )
    );
    DirectX::XMStoreFloat4x4(
        &m_PTF->ptf_WVP_ViewportSmall_Data.projection,
        XMMatrixTranspose(
            projectionMatrix_MAT
        )
    );




    DirectX::XMStoreFloat4x4(
        &m_PTF->ptf_WVP_ViewportLarge_Data.model,
        XMMatrixTranspose(
            loft_1stPerson_WorldTransformation // 1stPerson is rendered in the LARGE viewport;
        )
    );
    DirectX::XMStoreFloat4x4(
        &m_PTF->ptf_WVP_ViewportLarge_Data.view,
        XMMatrixTranspose(
            viewMatrix_1stPerson_MAT
        )
    );
    DirectX::XMStoreFloat4x4(
        &m_PTF->ptf_WVP_ViewportLarge_Data.projection,
        XMMatrixTranspose(
            projectionMatrix_MAT
        )
    );
}














void Hvy3DScene::Render()
{
    if (!this->m_Mandelpod->LoadingComplete())
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
    rasterizerstate_descr.AntialiasedLineEnable = FALSE;

    //  Generic solid rasterizer state: 

    rasterizerstate_descr.MultisampleEnable = TRUE;
    rasterizerstate_descr.FillMode = D3D11_FILL_SOLID;
    rasterizerstate_descr.FrontCounterClockwise = FALSE;
    rasterizerstate_descr.CullMode = D3D11_CULL_BACK;

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateRasterizerState(
        &rasterizerstate_descr,
        e_rasterizer_state_solid.ReleaseAndGetAddressOf()
    ));
}









void Hvy3DScene::CreateDeviceDependentResources()
{
    MSAA_TestDeviceSupport(); 

    this->m_PTF->CreateDeviceDependentResources();

    this->m_Mandelpod->CreateDeviceDependentResources();

    this->e_sphybox->CreateDeviceDependentResources();

    CreateRasterizerState();
}









void Hvy3DScene::ReleaseDeviceDependentResources()
{

    // TODO: 

}


