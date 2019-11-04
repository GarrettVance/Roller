//          
//          
//  File XModMandelpod.h   Resources and renderer for Mandelpod; 
//  Garrett Vance 20170219
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//

#pragma once

#include "..\Common\AlteredDevice.h"
#include "..\Common\StepTimer.h"
// #include "ParallelTransportFrame.h"
// #include "Sphybox.h"
#include "CWS_WaveFrontReader.h"


namespace HvyDX 
{
    struct VHG_MaterialUsageStruct
    {
        uint32_t        MaterialOrdinal;
        UINT            UsageCount;
    };

    struct VHG_MandelpodMatrixStruct
    {
        DirectX::XMFLOAT4X4     trxWVP;         //  World-View-Projection Transformation; 
        DirectX::XMFLOAT4X4     trxWorld;       //  World Transformation; 
        DirectX::XMFLOAT4X4     trxInvTposeWorld;   //  Inverse Transpose World Transformation; 
        DirectX::XMFLOAT4X4     trxInverseView;     //  Inverse View Transformation; 
        DirectX::XMUINT4        animator_counts;
    };

    class XModMandelpod
    {
    public:
        XModMandelpod(const std::shared_ptr<DX::DeviceResources>& deviceResources);

        void CreateDeviceDependentResources();
        void ReleaseDeviceDependentResources();
        void Update(DX::StepTimer const& timer);
        void Render(const DirectX::XMMATRIX& viewMat, const DirectX::XMMATRIX& projMat, int pSmall);

        bool LoadingComplete() { return mandelpod_loadingComplete; }

    private:
        void CreateColorSampler();
        void CreateEnvironmentSampler();
        void CreateVertexBufferWavefrontOBJ();
        void CreateRasterizerState();
        void DrawIndexedPerMaterial();

        void ComputeVertexTangents(
            std::vector<WaveFrontReader<DWORD>::WFR_Vertex> &   refVertices,
            std::vector<DWORD> const & ref_indices
        );



    public:  // TODO: revert to private: 

        DirectX::XMFLOAT4X4                                 mandelpod_worldMatrix_1stPerson_F4X4; 
        DirectX::XMFLOAT4X4                                 mandelpod_worldMatrix_3rdPerson_F4X4; 

    private:

        std::shared_ptr<DX::DeviceResources>                m_deviceResources;


        std::vector<VHG_MaterialUsageStruct>               *mandelpod_materialUsages;
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

        VHG_MandelpodMatrixStruct                           mandelpod_1stPerson_transformData;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                mandelpod_1stPerson_transformBuffer;

        VHG_MandelpodMatrixStruct                           mandelpod_3rdPerson_transformData;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                mandelpod_3rdPerson_transformBuffer;



    };

}
//  Closes namespace HvyDX;


