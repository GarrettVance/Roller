//          
//          
//  File DirectionCosineMatrix.cpp   Basis transformations for coordinate systems;
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




//  TODO:  change fov angle of the camera; 


//  TODO:  ensure that the camera "follow distance" 
//  maintains a constant length to prevent uncertainties in the render.









DirectX::XMMATRIX Hvy3DScene::DirectionCosineMatrix(
    DirectX::XMFLOAT3 p_Tangent, 
    DirectX::XMFLOAT3 p_Normal, 
    DirectX::XMFLOAT3 p_Binormal
)
{
    //  The Space Curve of the Lorenz Attractor has, at any instant, 
    //  a local frame with orthonormal basis 
    //  (curveTangentHat, curveNormalHat, curveBinormalHat); 

    //  
    //  Note: the DirectionCosineMatrix method uses XMVectorNegate 
    //  for purely aesthetic (rather than mathematical) reasons: 
    //  some mesh models look better in the altered orientation 
    //  due to how they were created in Blender. 
    //  

    XMVECTOR curveTangentHat = XMVector3Normalize(
            XMVectorSet(p_Tangent.x, p_Tangent.y, p_Tangent.z, 0.f)
    ); 
    curveTangentHat = XMVectorNegate(curveTangentHat);  // See Note above;

    XMVECTOR curveNormalHat = XMVector3Normalize(
        XMVectorSet(p_Normal.x, p_Normal.y, p_Normal.z, 0.f)
    );
    curveNormalHat = XMVectorNegate(curveNormalHat);  // See Note above;

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







void Hvy3DScene::CalculateViewMatrix_Following(
    DirectX::XMFLOAT3 const& p_Position, 
    DirectX::XMFLOAT3 const& p_Tangent, 
    DirectX::XMFLOAT3 const& p_Normal,
    DirectX::XMMATRIX const& p_RotationMatrix
)
{
    //  TODO:  use new XMVECTOR member transported_normal: 


    //  
    //  The curve Normal point inward towards the center of curvature, 
    //  so values of cameraNormalDistance must be chosen < 0 (i.e. negative). 
    //      
    float cameraNormalDistance = -2.f;

    //      
    //  CameraLookAheadDistance between 2.f and 4.f; 
    //      
    float cameraLookAheadDistance = 5.f; //  3.f;

    //      
    //  CameraTrailingDistance must be < 0 (negative) to put camera behind mesh model. 
    //  CameraTrailingDistance between -2.f and -3.f; 
    //      
    float cameraTrailingDistance = -3.f;

    XMVECTOR translatedXMV = XMVectorSet(p_Position.x - 2.f, p_Position.y, p_Position.z + e_ChiralityZOffset, 1.0f);


    //  "forwards" is synonymous with tangent: "backwards" can be obtained from "forwards": 
    XMVECTOR normalizedTangentXMV = XMVector3Normalize(
        XMVectorSet(p_Tangent.x, p_Tangent.y, p_Tangent.z, 0.f)  // homogeneous w_component = 0; 
    );

    XMVECTOR normalizedNormalXMV = XMVector3Normalize(
        XMVectorSet(p_Normal.x, p_Normal.y, p_Normal.z, 0.f)  // homogeneous w_component = 0; 
    );

    XMVECTOR cameraPosition = 
        translatedXMV + 
        XMVectorScale(normalizedTangentXMV, cameraTrailingDistance) + 
        XMVectorScale(normalizedNormalXMV, cameraNormalDistance);

    XMVECTOR cameraLookAt = translatedXMV + XMVectorScale(normalizedTangentXMV, cameraLookAheadDistance);

    cameraLookAt = cameraPosition + XMVectorScale(normalizedTangentXMV, cameraLookAheadDistance);

    //  experimental XMVECTOR worldUpDirection = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMVECTOR worldUpDirection = XMVectorNegate(normalizedNormalXMV);

    if (e_ViewMatrixFixed == true)
    {
        cameraPosition = XMVectorSet(40.f, 2.f, 60.f, 1.0f);
        cameraLookAt = XMVectorSet(20.f, 0.5f, 66.f, 1.0f);
        worldUpDirection = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); 
    }

    if (!XMVector3Equal(cameraLookAt, XMVectorZero()))
    {
        XMMATRIX mView = XMMatrixLookAtLH(cameraPosition, cameraLookAt, worldUpDirection); // Left-handed;
        XMStoreFloat4x4(&m_mView, mView);
    }
}







void Hvy3DScene::Update(DX::StepTimer const& timer)
{
    DirectX::Keyboard::State        kb = kmi_keyboard->GetState();

    if (kb.F5 && (e_ViewMatrixFixed == true))
    {
        e_ViewMatrixFixed = false;
    }

    if (kb.F6 && (e_ViewMatrixFixed == false))
    {
        e_ViewMatrixFixed = true;
    }

    if (kb.F7 && (e_UsingMSAA == true))
    {
        e_UsingMSAA = false;
    }

    if (kb.F8 && (e_UsingMSAA == false))
    {
        e_UsingMSAA = true;
    }

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


    static double time_now = 0.00;
    static double time_prior = 0.00; 

    time_prior = time_now;
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

        if (idxUpdateCall % 4 == 0)
        {
            idxSpaceCurveElt += 1;
            if (idxSpaceCurveElt + 2 == card) idxSpaceCurveElt = 0; // wrap-around;
        }
    }

    // Calculate the correct rotation matrix to align the mandelPod: 

    XMMATRIX mCameraRot = DirectionCosineMatrix(spaceCurveTangent, spaceCurveNormal, spaceCurveBinormal); 

    CalculateViewMatrix_Following(
        spaceCurvePos,
        spaceCurveTangent,
        spaceCurveNormal,
        mCameraRot
    ); 

    //      
    //  World Transformation of the Space Pod (aka MandelPod)
    //  Apply scaling to the Space Pod prior to translation: 
    //          
    float s5 = (e_ViewMatrixFixed == true) ? 4.f : 3.f;
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

#ifdef GHV_OPTION_LATER
    CalculateViewMatrix(spaceCurvePos, spaceCurveTangent, spaceCurveNormal, cameraRotation);
#endif

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
        XMMATRIX mView = XMLoadFloat4x4(&m_mView);
        XMMATRIX mProj = XMLoadFloat4x4(&m_ProjectionMatrix);

        this->m_PTF->Update(
            lorenzAttractorWorldTransformation,
            mView,
            mProj 
        );
    }
}







