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



        void CalculateViewMatrix(
            DirectX::XMFLOAT3 const& p_Position, 
            DirectX::XMFLOAT3 const& p_Tangent, 
            DirectX::XMFLOAT3 const& p_Normal
        ); 





        DirectX::XMMATRIX DirectionCosineMatrix(
            DirectX::XMFLOAT3 p_Tangent,
            DirectX::XMFLOAT3 p_Normal,
            DirectX::XMFLOAT3 p_Binormal
        );

		void Update(DX::StepTimer const& timer);

		void Render();

        void RenderMandelPod(); 


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
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>		e_rasterizer_state_solid;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>		e_rasterizer_state_wireframe;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>		e_rasterizer_state_mandelpod;

        bool                                                e_ViewMatrixFixed; 
        float                                               e_ChiralityZOffset; // TODO: cleanup;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    e_srv_mirror; 
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    e_srv_black;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    e_srv_normal;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    e_srv_environment;

        std::vector<VHG_MaterialUsage_struct>              *e_vect_material_usages;

		Microsoft::WRL::ComPtr<ID3D11InputLayout>	        m_waveFrontInputLayoutTNB;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		        m_vertexBuffer;
		uint32	                                            m_vertexCount;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		        m_indexBuffer;
		uint32	                                            m_indexCount;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	        m_vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	        m_pixelShader;
		bool                                                m_loadingComplete;

        DirectX::XMMATRIX                                   e_spacePodWorldTransformation; 
        DirectX::XMMATRIX                                   e_xmmatrix_view_trx;
        DirectX::XMMATRIX                                   e_xmmatrix_projection_trx;

        Microsoft::WRL::ComPtr<ID3D11Buffer>                e_conbuf_Transform_buffer;
        VHG_conbuf_MVPA_struct                              e_conbuf_Transform_data; // TODO: rename;

        Microsoft::WRL::ComPtr<ID3D11SamplerState>          e_colorSampler;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>          e_normalSampler;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>          e_environmentSampler;

        std::unique_ptr<DirectX::Keyboard>                  kmi_keyboard;
        std::unique_ptr<DirectX::Mouse>                     kmi_mouse;

        DirectX::XMVECTOR                                   cameraPosition;
	};

}
//  Closes namespace HvyDX;


