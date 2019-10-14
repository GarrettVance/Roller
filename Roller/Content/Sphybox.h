//          
//          
//          
//  File Sphybox.h   Spherical Skybox Utility
//  Garrett Vance 20170219
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//              
//          

#pragma once

#include "..\Common\DeviceResources.h"
#include "TexCoSphere.h"



namespace HvyDX
{
    struct VHG_Sphybox_MVP_Struct
    {
        DirectX::XMFLOAT4X4         ModelViewProjection;
    };


    class Sphybox
    {
    public:
        Sphybox(const std::shared_ptr<DX::DeviceResources>& deviceResources);

        void CreateDeviceDependentResources();

        bool LoadingComplete() { return sphybox_loadingComplete; }

        void Render(const DirectX::XMFLOAT3& eyePos, const DirectX::XMMATRIX& viewMat, const DirectX::XMMATRIX& projMat);

    private:
        void CreateDepthStencilState();

        void CreateCubeSamplerState();

        void CreateRasterizerState();

    private:
        std::shared_ptr<DX::DeviceResources>                    m_deviceResources;

        std::unique_ptr<HvyDX::TexCoSphere>                     sphybox_sphere;

        Microsoft::WRL::ComPtr<ID3D11InputLayout>               sphybox_inputLayout;

        uint32_t                                                sphybox_vertex_count;
        uint32_t                                                sphybox_index_count;




        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> ghv_DepthStencilState;  //  <---- here!!!!  TODO: 


        Microsoft::WRL::ComPtr<ID3D11VertexShader>              sphybox_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState>           sphybox_rasterizerState;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>               sphybox_pixelShader;


        VHG_Sphybox_MVP_Struct                                  sphybox_MVP_ConbufData;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                    sphybox_MVP_ConbufBuffer;


        Microsoft::WRL::ComPtr<ID3D11SamplerState>              sphybox_cube_sampler_state; 
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>        sphybox_cube_srv;

        bool                                                    sphybox_loadingComplete;
    };
}
//  Closes namespace HvyDX; 




