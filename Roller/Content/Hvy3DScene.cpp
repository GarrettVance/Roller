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
    m_PTF = std::make_unique<HvyDX::XModLorenzLoft>(deviceResources);

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
    if (
        (!this->m_Mandelpod->LoadingComplete()) || 
        (!this->m_PTF->LoadingComplete()) || 
        (!this->e_sphybox->LoadingComplete())
    )
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

    static uint32_t idxSpaceCurveElt = 0;
    static double time_prior = timer.GetTotalSeconds();
    double time_now = timer.GetTotalSeconds();
    double fElapsedTime = time_now - time_prior;

    if (!m_PTF->loft_axons.empty())
    {
        uint32_t card = (uint32_t)(m_PTF->loft_axons.size());
        if (fElapsedTime > 0.03f)
        {
            time_prior = time_now;
            (idxSpaceCurveElt + 1 < card) ? idxSpaceCurveElt++ : idxSpaceCurveElt = 0;
        }
    }

    XMFLOAT3 spaceCurvePos = this->m_PTF->loft_axons.at(idxSpaceCurveElt).axon_position_r;
    XMFLOAT3 spaceCurveTangent = this->m_PTF->loft_axons.at(idxSpaceCurveElt).axon_tangent_drdt; 
    XMFLOAT3 spaceCurveNormal = this->m_PTF->loft_axons.at(idxSpaceCurveElt).axon_normal; 
    XMFLOAT3 spaceCurveBinormal = this->m_PTF->loft_axons.at(idxSpaceCurveElt).axon_binormal; 


    XMMATRIX viewMatrix_1stPerson_MAT = CalculateViewMatrix_1stPersonFollowing(spaceCurvePos, spaceCurveTangent, spaceCurveNormal); 
    XMStoreFloat4x4(&viewMatrix_1stPerson_F4X4, viewMatrix_1stPerson_MAT); // TODO: what happened to transpose ??? 

    XMMATRIX viewMatrix_3rdPerson_MAT = CalculateViewMatrix_3rdPerson(); 
    XMStoreFloat4x4(&viewMatrix_3rdPerson_F4X4, viewMatrix_3rdPerson_MAT); // TODO: what happened to transpose ??? 

    XMMATRIX projectionMatrix_MAT = XMLoadFloat4x4(&m_ProjectionMatrix);

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //  World Transformation of the Lorenz Attractor: 
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    const float xlat_x_3rdPerson = 0.f;   //  Use 0.f; 
    const float xlat_y_3rdPerson = 4.f;   //  Use 4.f; 
    const float xlat_z_3rdPerson = -2.f;  //  Use -2.f; 
    XMMATRIX Scaling3rdP = XMMatrixIdentity();
    XMMATRIX attitude_3rdPerson = XMMatrixIdentity();


    XMMATRIX loft_1stPerson_Translation = XMMatrixTranslation(-2.f, 0.f, e_ZOffset);  
    XMMATRIX loft_1stPerson_WorldTransformation = loft_1stPerson_Translation;

    XMMATRIX loft_3rdPerson_Translation = XMMatrixTranslation(xlat_x_3rdPerson, xlat_y_3rdPerson, xlat_z_3rdPerson); 
    XMMATRIX loft_3rdPerson_WorldTransformation = loft_3rdPerson_Translation * Scaling3rdP * attitude_3rdPerson; 

    m_PTF->Store(
        loft_3rdPerson_WorldTransformation,  // 3rdPerson is rendered in small viewport;
        viewMatrix_3rdPerson_MAT,
        projectionMatrix_MAT, 
        1
    );

    m_PTF->Store(
        loft_1stPerson_WorldTransformation,  // 1stPerson is rendered in the LARGE viewport;
        viewMatrix_1stPerson_MAT,
        projectionMatrix_MAT,
        -1
    );

#ifdef _DEBUG
    float accumulatedRadians = (float)timer.GetTotalSeconds() * XMConvertToRadians(45.f);  
    float properRadians = static_cast<float>(fmod(accumulatedRadians, DirectX::XM_2PI)); 
    XMMATRIX spacePodSpin = XMMatrixRotationX(properRadians);  
#else
    XMMATRIX spacePodSpin = XMMatrixIdentity();
#endif

    //  World transformation of Mandelpod, 1st person (major viewport): 

    XMMATRIX spacePodXlat_1stPerson = XMMatrixTranslation(spaceCurvePos.x - 2.f, spaceCurvePos.y, spaceCurvePos.z + e_ZOffset);

#ifdef _DEBUG
    XMMATRIX spacePodScaling1stPerson = XMMatrixScaling(0.7f, 0.7f, 0.7f);
#else
    XMMATRIX spacePodScaling1stPerson = XMMatrixScaling(4.f, 4.f, 4.f);
#endif

    XMMATRIX spacePodRotation = DirectionCosineMatrix(spaceCurveTangent, spaceCurveNormal, spaceCurveBinormal); 

    XMMATRIX mandelpod_worldMatrix_1stPerson_MAT = spacePodScaling1stPerson * spacePodSpin * spacePodRotation * spacePodXlat_1stPerson;

    XMStoreFloat4x4(
        &this->m_Mandelpod->mandelpod_worldMatrix_1stPerson_F4X4, 
        mandelpod_worldMatrix_1stPerson_MAT // TODO: transpose ??? 
    );

    //  World transformation of Mandelpod, 3rd person (minor viewport): 

#ifdef _DEBUG
    XMMATRIX spacePodScaling3rdPerson = XMMatrixScaling(1.6f, 1.6f, 1.6f);
#else
    // XMMATRIX spacePodScaling3rdPerson = XMMatrixScaling(5.f, 5.f, 5.f); 
    XMMATRIX spacePodScaling3rdPerson = XMMatrixScaling(7.f, 7.f, 7.f); 
#endif

    XMVECTOR rawXMV = XMVectorSet(spaceCurvePos.x, spaceCurvePos.y, spaceCurvePos.z, 1.f); // TODO: homogeneous;
    XMVECTOR actualXMV = XMVector3Transform(rawXMV, loft_3rdPerson_WorldTransformation); 
    XMMATRIX spacePodXlat_3rdPerson = XMMatrixTranslation( XMVectorGetX(actualXMV), XMVectorGetY(actualXMV), XMVectorGetZ(actualXMV) );


    XMMATRIX mandelpod_worldMatrix_3rdPerson_MAT = 
        spacePodScaling3rdPerson * 
        spacePodSpin * 
        spacePodRotation *   // DON'T TRANSFORM "spacePodRotation". 
        spacePodXlat_3rdPerson * 
        attitude_3rdPerson;

    XMFLOAT4X4* world3rdPerson_F4X4 = &this->m_Mandelpod->mandelpod_worldMatrix_3rdPerson_F4X4; 
    XMStoreFloat4x4(world3rdPerson_F4X4, mandelpod_worldMatrix_3rdPerson_MAT);
}




void Hvy3DScene::Render()
{
    if (
        (!this->m_Mandelpod->LoadingComplete()) || 
        (!this->m_PTF->LoadingComplete()) || 
        (!this->e_sphybox->LoadingComplete())
    )
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


