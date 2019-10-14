//          
//          
//  File TexCoSphere.h   Meshes a uv-textured sphere;
//  Garrett Vance 20170219
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//              
//          
//               

#pragma once

#include "..\Common\DeviceResources.h"


namespace HvyDX
{

	class VHG_Spherolux
	{
    public: 
        VHG_Spherolux(): 
            position(DirectX::XMFLOAT3(0.f, 0.f, 0.f)), 
            normal(DirectX::XMFLOAT3(0.f, 0.f, 0.f)), 
            texcoGlobal(DirectX::XMFLOAT2(0.f, 0.f)),
            texcoLocal(DirectX::XMFLOAT2(0.f, 0.f)) 
        {}

        VHG_Spherolux(
            DirectX::XMFLOAT3 pPosition,  
            DirectX::XMFLOAT3 pNormal,  
            DirectX::XMFLOAT2 pTexcoGlobal,
            DirectX::XMFLOAT2 pTexcoLocal
        ): 
            position(DirectX::XMFLOAT3(pPosition.x, pPosition.y, pPosition.z)), 
            normal(DirectX::XMFLOAT3(pNormal.x, pNormal.y, pNormal.z)), 
            texcoGlobal(DirectX::XMFLOAT2(pTexcoGlobal.x, pTexcoGlobal.y)),
            texcoLocal(DirectX::XMFLOAT2(pTexcoLocal.x, pTexcoLocal.y)) 
        {}


        //  ghv : TODO: copy ctor...

		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 texcoGlobal;
		DirectX::XMFLOAT2 texcoLocal;
	};


    struct VHG_asdf_MVP_Struct
    {
        DirectX::XMFLOAT4X4         ModelViewProjection;
    };


    class TexCoSphere
    {
    public:
        TexCoSphere(const std::shared_ptr<DX::DeviceResources>& deviceResources, DirectX::XMFLOAT3 p_centre, float p_radius, uint32_t p_precision);

        VHG_Spherolux TexCoSphere::ComputeTextureCoordinates(
            float pThetaColatitude,
            float pLambdaLongitude,
            DirectX::XMFLOAT3 const & pSCentre,
            float pSRadius,
            float uLocal,
            float vLocal
        );

        void LoadVertexBuffer(uint32_t* pVBCard, uint32_t* pIBCard);


    public:

        Microsoft::WRL::ComPtr<ID3D11Buffer>                    tcs_vertex_buffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                    tcs_index_buffer;


    private:
        std::shared_ptr<DX::DeviceResources>                    tcs_deviceResources;
        DirectX::XMFLOAT3                                       tcs_centre; 
        float                                                   tcs_radius;
        uint32_t                                                tcs_precision;


    };
}
//  Closes namespace HvyDX; 







