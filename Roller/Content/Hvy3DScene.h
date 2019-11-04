//          
//          
//  File Hvy3DScene.h   Main scene renderer; 
//  Garrett Vance 20170219
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//              
//          
//               


#pragma once

#include "..\Common\AlteredDevice.h"
#include "..\Common\StepTimer.h"
#include "ParallelTransportFrame.h"
#include "XModMandelpod.h"
#include "Sphybox.h"
#include "CWS_WaveFrontReader.h"





namespace HvyDX 
{

    struct VHG_MaterialUsage_struct
    {
        uint32_t        MaterialOrdinal;
        UINT            UsageCount;
    };



    struct VHG_conbuf_MVPA_struct
    {
        DirectX::XMFLOAT4X4     trxWVP;         //  World-View-Projection Transformation; 
        DirectX::XMFLOAT4X4     trxWorld;       //  World Transformation; 
        DirectX::XMFLOAT4X4     trxInvTposeWorld;   //  Inverse Transpose World Transformation; 
        DirectX::XMFLOAT4X4     trxInverseView;     //  Inverse View Transformation; 
        DirectX::XMUINT4        animator_counts;
    };





    /*
    struct KMI_InitialView_struct
    {
        DirectX::XMFLOAT3           f3_camera_position;
        float                       f_look_at_pitch;
        float                       f_look_at_yaw;
    };
    */









    class Hvy3DScene
    {
    public:
        Hvy3DScene(const std::shared_ptr<DX::DeviceResources>& deviceResources);

        void CreateDeviceDependentResources();
        void CreateWindowSizeDependentResources();
        void ReleaseDeviceDependentResources();
        void Update(DX::StepTimer const& timer);
        void Render();


        void MSAA_CreateWindowSizeDepResources(); 
        void MSAA_Render(); 
        void MSAA_TestDeviceSupport();


        void CalculateViewMatrix_Following(
            DirectX::XMFLOAT3 const& p_Position, 
            DirectX::XMFLOAT3 const& p_Tangent, 
            DirectX::XMFLOAT3 const& p_Normal,
            DirectX::XMMATRIX const& p_RotationMatrix
        ); 

        DirectX::XMMATRIX DirectionCosineMatrix(
            DirectX::XMFLOAT3 p_Tangent,
            DirectX::XMFLOAT3 p_Normal,
            DirectX::XMFLOAT3 p_Binormal
        );



    private:
        void CreateRasterizerState(); // Generic RasterizerState common to various objects;




    private:
        std::shared_ptr<DX::DeviceResources>                m_deviceResources;

        std::unique_ptr<HvyDX::ParallelTransportFrame>      m_PTF;
        std::unique_ptr<HvyDX::XModMandelpod>               m_Mandelpod;
        std::unique_ptr<HvyDX::Sphybox>                     e_sphybox;

        //  MSAA resources
        //====================================================================
        Microsoft::WRL::ComPtr<ID3D11Texture2D>             e_msaaRenderTarget;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView>      e_msaaRenderTargetView;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView>      e_msaaDepthStencilView;
        unsigned int                                        e_MSAASampleCount;
        bool                                                e_UsingMSAA;




        Microsoft::WRL::ComPtr<ID3D11RasterizerState>       e_rasterizer_state_solid; // TODO: which class? 



        float                                               e_ZOffset; // TODO: cleanup; applies to LorenzLoft, and thus mandelpod;




        // Camera attributes: 

        DirectX::XMFLOAT3                                   m_vEye; 
        DirectX::XMFLOAT3                                   m_vLookAt; // LookAt position; 
        DirectX::XMFLOAT4X4                                 viewMatrix_1stPerson_F4X4;
        DirectX::XMFLOAT4X4                                 viewMatrix_3rdPerson_F4X4;
        DirectX::XMFLOAT4X4                                 m_ProjectionMatrix;



        std::unique_ptr<DirectX::Keyboard>                  kmi_keyboard;
        std::unique_ptr<DirectX::Mouse>                     kmi_mouse;
    };

}
//  Closes namespace HvyDX;


