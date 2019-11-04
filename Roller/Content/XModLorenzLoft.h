//                      
//                      

#pragma once

#include "..\Common\AlteredDevice.h"
#include "..\Common\StepTimer.h"



namespace HvyDX
{
    struct VHG_Axonodromal_Vertex
    {
        DirectX::XMFLOAT3       axon_position_r;
        DirectX::XMVECTOR       axon_position_V;

        float                   axon_elapsed_time;
        DirectX::XMFLOAT3       axon_tangent_drdt;
        DirectX::XMFLOAT3       axon_d2rdt2;


        // 
        DirectX::XMFLOAT3       axon_normal; // consumed in Hvy3DScene.cpp; 
        DirectX::XMVECTOR       transported_normal; 
        // 


        DirectX::XMFLOAT3       axon_binormal; 
    };


    struct VHG_Vertex_PosTex
    {
        DirectX::XMFLOAT3       e_pos;
        // DirectX::XMFLOAT3       e_normal; // ghv: added 20191016 0817;  TODO: change inputLayout to match;
        DirectX::XMFLOAT2       e_texco;
        uint32_t                  e_segment_id;
    };


    struct VHG_LoftMVPStruct
    {
        DirectX::XMFLOAT4X4     model;
        DirectX::XMFLOAT4X4     view;
        DirectX::XMFLOAT4X4     projection;
        DirectX::XMUINT4        animator_count;
    };



    class XModLorenzLoft
    {
    public:
        XModLorenzLoft(const std::shared_ptr<DX::DeviceResources>& deviceResources);

        void CreateDeviceDependentResources();
        void ReleaseDeviceDependentResources();

        /*
        void Update(
            DirectX::XMMATRIX        const&           p_ParallelTransportFrameWorldMatrix,
            DirectX::XMMATRIX        const&           p_parentSceneViewMatrix,
            DirectX::XMMATRIX        const&           p_parentSceneProjectionMatrix
        );
        */


        void Render(const DirectX::XMMATRIX& viewMat, const DirectX::XMMATRIX& projMat, int pSmall);


        bool LoadingComplete() { return loft_loadingComplete; }


        void Create_Input_Layout(
            const std::vector<byte>& p_byte_vector
        ); 

        void Create_Vertex_Buffer(
            std::vector<VHG_Vertex_PosTex>  *p_vect_vertices,
            ID3D11Buffer** p_buffer_object
        );

        void HansonParallelTransportFrame(); 

        void FiniteDifferences_Derivative();

        size_t ReadLorenzDataFile(); 


    public:  //  TODO: revert to private;

        std::vector<HvyDX::VHG_Axonodromal_Vertex>          loft_axons; 

        Microsoft::WRL::ComPtr<ID3D11Buffer>                loft_WVP_ViewportMinor_Buffer;
        VHG_LoftMVPStruct                                   loft_WVP_ViewportMinor_Data;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                loft_WVP_ViewportMajor_Buffer;
        VHG_LoftMVPStruct                                   loft_WVP_ViewportMajor_Data;


    private: 
        std::shared_ptr<DX::DeviceResources>                m_deviceResources;

        uint32_t                                            loft_axon_arc_density;
        uint32_t                                            loft_tube_facets;
        float                                               loft_tube_radius;

        Microsoft::WRL::ComPtr<ID3D11InputLayout>           loft_inputLayout;

        Microsoft::WRL::ComPtr<ID3D11Buffer>                loft_vertex_buffer_1_buffer; // for spokes;
        uint32_t                                            loft_vertex_buffer_1_count;  // for spokes;

        Microsoft::WRL::ComPtr<ID3D11Buffer>                loft_vertex_buffer_2_buffer;
        uint32_t                                            loft_vertex_buffer_2_count;



        Microsoft::WRL::ComPtr<ID3D11VertexShader>          loft_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>           loft_minorPixelShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>           loft_majorPixelShader;


        Microsoft::WRL::ComPtr<ID3D11SamplerState>          loft_texture_sampler_state;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    loft_texture_srv;

        bool                                                loft_loadingComplete;
    };
}
//  Closes namespace HvyDX; 



                                                                
