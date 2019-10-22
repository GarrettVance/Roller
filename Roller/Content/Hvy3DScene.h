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





    struct KMI_InitialView_struct
    {
        DirectX::XMFLOAT3           f3_camera_position;
        float                       f_look_at_pitch;
        float                       f_look_at_yaw;
    };









    class Hvy3DScene
    {
    public:
        Hvy3DScene(const std::shared_ptr<DX::DeviceResources>& deviceResources);

        void CreateDeviceDependentResources();
        void CreateWindowSizeDependentResources();
        void ReleaseDeviceDependentResources();

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

        void Update(DX::StepTimer const& timer);

        void Render();

        void RenderMandelPodViewportSmall(); 
        void RenderMandelPodViewportLarge(); 


    private:
        void CreateColorSampler();

        void CreateEnvironmentSampler();

        void ComputeVertexTangents(
            std::vector<WaveFrontReader<DWORD>::WFR_Vertex> &   refVertices,
            std::vector<DWORD> const & ref_indices
        );

        void CreateVertexBufferWavefrontOBJ(void);

        void CreateRasterizerState();

        void DrawIndexedPerMaterial(void);

    private:
        std::shared_ptr<DX::DeviceResources>                m_deviceResources;

        std::unique_ptr<HvyDX::ParallelTransportFrame>      m_PTF;

        std::unique_ptr<HvyDX::Sphybox>                     e_sphybox;

        //  MSAA resources
        //====================================================================
        Microsoft::WRL::ComPtr<ID3D11Texture2D>             e_msaaRenderTarget;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView>      e_msaaRenderTargetView;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView>      e_msaaDepthStencilView;
        unsigned int                                        e_MSAASampleCount;
        bool                                                e_UsingMSAA;



        Microsoft::WRL::ComPtr<ID3D11RasterizerState>       e_rasterizer_state_solid;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState>       e_rasterizer_state_wireframe;

        // retired bool                                                e_View3rdPerson; 

        float                                               e_ZOffset; // TODO: cleanup;


        std::vector<VHG_MaterialUsage_struct>              *mandelpod_materialUsages;
        Microsoft::WRL::ComPtr<ID3D11InputLayout>           mandelpod_waveFrontInputLayoutTNB;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                mandelpod_vertexBuffer;
        uint32                                              mandelpod_vertexCount;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                mandelpod_indexBuffer;
        uint32                                              mandelpod_indexCount;
        Microsoft::WRL::ComPtr<ID3D11VertexShader>          mandelpod_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState>       mandelpod_rasterizerState;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>           mandelpod_pixelShader;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    mandelpod_srv_mirror; 
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    mandelpod_srv_black;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    mandelpod_srv_normal;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    mandelpod_srv_environment;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>          mandelpod_colorSampler;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>          mandelpod_normalSampler;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>          mandelpod_environmentSampler;
        bool                                                mandelpod_loadingComplete;

        VHG_conbuf_MVPA_struct                              mandelpod_1stPerson_transformData;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                mandelpod_1stPerson_transformBuffer;

        VHG_conbuf_MVPA_struct                              mandelpod_3rdPerson_transformData;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                mandelpod_3rdPerson_transformBuffer;


        DirectX::XMFLOAT4X4                                 mandelpod_worldMatrix_1stPerson_F4X4; 
        DirectX::XMFLOAT4X4                                 mandelpod_worldMatrix_3rdPerson_F4X4; 

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


