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
// #include "ParallelTransportFrame.h"
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
    XMVECTOR cameraPosition3rdPerson = XMVectorSet(-20.f, 0.f, -4.f, 1.0f);
    XMVECTOR cameraLookAt3rdPerson = XMVectorSet(10.f, 0.0f, 36.f, 1.0f); //
    XMVECTOR worldUpDirection3rdPerson = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); 

    XMMATRIX viewMatrix_3rdPerson_MAT = XMMatrixLookAtLH(cameraPosition3rdPerson, cameraLookAt3rdPerson, worldUpDirection3rdPerson); // Left-handed;
    XMStoreFloat4x4(&viewMatrix_3rdPerson_F4X4, viewMatrix_3rdPerson_MAT);

    //  
    //  The curve Normal points inward towards the center of curvature, 
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

    XMVECTOR translatedXMV = XMVectorSet(p_Position.x - 2.f, p_Position.y, p_Position.z + e_ZOffset, 1.0f);

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


#if 3 == 3  
    // Either of these will work...
    XMVECTOR cameraLookAt = translatedXMV + XMVectorScale(normalizedTangentXMV, cameraLookAheadDistance);
#else 
    XMVECTOR cameraLookAt = cameraPosition + XMVectorScale(normalizedTangentXMV, cameraLookAheadDistance);
#endif


    XMVECTOR worldUpDirection = XMVectorNegate(normalizedNormalXMV);
    
    XMMATRIX viewMatrix_1stPerson_MAT = XMMatrixLookAtLH(cameraPosition, cameraLookAt, worldUpDirection); // Left-handed;
    XMStoreFloat4x4(&viewMatrix_1stPerson_F4X4, viewMatrix_1stPerson_MAT);
}





