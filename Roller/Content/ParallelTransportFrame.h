//                      
//                      
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                      
//                ghv : Garrett Vance : 20180607
//                      
//                      
//          Implementation of Parallel Transport Frames
//          following Andrew J. Hanson's 1995 paper 
//          "Parallel Transport Approach to Curve Framing".
//                      
//                      
//            DirectX 11 Application for Windows 10 UWP.
//                      
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
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


    struct VHG_ConBuf_MVP_Struct
    {
        DirectX::XMFLOAT4X4     model;
        DirectX::XMFLOAT4X4     view;
        DirectX::XMFLOAT4X4     projection;
        DirectX::XMUINT4        animator_count;
    };



    class ParallelTransportFrame
    {
    public:
        ParallelTransportFrame(const std::shared_ptr<DX::DeviceResources>& deviceResources);

        void CreateDeviceDependentResources();
        void ReleaseDeviceDependentResources();

        /*
        void Update(
            DirectX::XMMATRIX        const&           p_ParallelTransportFrameWorldMatrix,
            DirectX::XMMATRIX        const&           p_parentSceneViewMatrix,
            DirectX::XMMATRIX        const&           p_parentSceneProjectionMatrix
        );
        */

        void RenderViewportSmall();
        void RenderViewportLarge();

        bool            LoadingComplete() { return ptf_loadingComplete; }


        void Create_Input_Layout(
            const std::vector<byte>& p_byte_vector
        ); 

        void Create_Vertex_Buffer(
            std::vector<VHG_Vertex_PosTex>  *p_vect_vertices,
            ID3D11Buffer** p_buffer_object
        );

        void HansonParallelTransportFrame(); 

        void gv_finite_differences();

        size_t gv_read_lorenz_data_file(); 

    public:

        std::vector<HvyDX::VHG_Axonodromal_Vertex>          ptf_axons; 

        Microsoft::WRL::ComPtr<ID3D11Buffer>                ptf_WVP_ViewportSmall_Buffer;
        VHG_ConBuf_MVP_Struct                               ptf_WVP_ViewportSmall_Data;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                ptf_WVP_ViewportLarge_Buffer;
        VHG_ConBuf_MVP_Struct                               ptf_WVP_ViewportLarge_Data;


    private: 
        std::shared_ptr<DX::DeviceResources>                m_deviceResources;

        uint32_t                                            ptf_axon_arc_density;
        uint32_t                                            ptf_tube_facets;
        float                                               ptf_tube_radius;

        Microsoft::WRL::ComPtr<ID3D11InputLayout>           ptf_inputLayout;

        Microsoft::WRL::ComPtr<ID3D11Buffer>                ptf_vertex_buffer_1_buffer; // for spokes;
        uint32_t                                            ptf_vertex_buffer_1_count;  // for spokes;

        Microsoft::WRL::ComPtr<ID3D11Buffer>                ptf_vertex_buffer_2_buffer;
        uint32_t                                            ptf_vertex_buffer_2_count;



        Microsoft::WRL::ComPtr<ID3D11VertexShader>          ptf_vertexShader;
        

        Microsoft::WRL::ComPtr<ID3D11PixelShader>           ptf_pixelShader;


        Microsoft::WRL::ComPtr<ID3D11SamplerState>          ptf_loft_texture_sampler_state;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    ptf_loft_texture_srv;

        bool                                                ptf_loadingComplete;
    };
}
//  Closes namespace HvyDX; 



                                                                
