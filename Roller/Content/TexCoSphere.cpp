//          
//          
//  File TexCoSphere.cpp   Meshes a uv-textured sphere;
//  Garrett Vance 20170219
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//              
//          
//               



#include "pch.h"
#include "..\Common\DirectXHelper.h"
#include "TexCoSphere.h"

using namespace HvyDX; 
using namespace DirectX;
using namespace Windows::Foundation;



TexCoSphere::TexCoSphere(const std::shared_ptr<DX::DeviceResources>& deviceResources, DirectX::XMFLOAT3 p_centre, float p_radius, uint32_t p_precision)
    :
    tcs_deviceResources(deviceResources),
    tcs_centre(p_centre),
    tcs_radius(p_radius),
    tcs_precision(p_precision)
{}




VHG_Spherolux TexCoSphere::ComputeTextureCoordinates(
    float pThetaColatitude,
    float pLambdaLongitude,
    DirectX::XMFLOAT3 const & pSCentre, 
    float pSRadius, 
    float uLocal, 
    float vLocal
)
{
    XMFLOAT3 eNml = XMFLOAT3(0.f, 0.f, 0.f);  

    eNml.x = sin(pThetaColatitude) * cos(pLambdaLongitude);           
    eNml.z = sin(pThetaColatitude) * sin(pLambdaLongitude);           
    eNml.y = cos(pThetaColatitude);


    XMFLOAT3 posn = XMFLOAT3(0.f, 0.f, 0.f);  

    posn.x = pSCentre.x + pSRadius * eNml.x;        
    posn.z = pSCentre.z + pSRadius * eNml.z;        
    posn.y = pSCentre.y + pSRadius * eNml.y;



#if 3 == 4
    float psi = -6.f * std::atanh(sin(pLambdaLongitude));  // experimental undo;

    posn.x = pSCentre.x + eNml.x * (3.f * cos(pThetaColatitude) / std::cosh(psi)) * pSRadius;

    posn.y = pSCentre.y - eNml.y * (sin(pThetaColatitude) / std::cosh(psi)) * pSRadius;

    posn.z = pSCentre.z + eNml.z * (2.f * sinh(psi) / std::cosh(psi)) * pSRadius;
#endif 






    XMFLOAT2 texGlobal = XMFLOAT2(0.f, 0.f); 


    //  Classic:  texGlobal.x = 2.f * pLambdaLongitude / (float)DirectX::XM_2PI;  // aka texco_u;
    //  Classic:  texGlobal.y = 1.f - pThetaColatitude / (float)DirectX::XM_PI; // aka texco_v; 

    XMFLOAT2 texLocal = XMFLOAT2(uLocal, vLocal); 
            

    //  
    //  Polar coordinates <polco_radius, polco_phi> 
    //  are used to identify points on the unit disk D: 
    //  
    //  0 <= polco_radius <= 1 and 
    //  -pi < polco_phi <= +pi;
    //      

    float polco_phi = pLambdaLongitude - DirectX::XM_PI / 2.f; 


#if 1 == 1
    float polco_radius = sin(pThetaColatitude / 2.f); // Equal-area Projection (Lambert Azimuthal); 

#else

    //  ghv: The factor of 2.f multiplying tan() is aethstetic: 

    float polco_radius = 2.f * tan(pThetaColatitude / 2.f); // Modified Conformal Projection (Altered Stereographic); 
#endif

    float u_diskPolar = polco_radius * cos(polco_phi);  
    float v_diskPolar = polco_radius * sin(polco_phi);  

    texGlobal.x = u_diskPolar;
    texGlobal.y = v_diskPolar;

    return VHG_Spherolux(posn, eNml, texGlobal, texLocal); 
}





void TexCoSphere::LoadVertexBuffer(uint32_t* pVBCard, uint32_t* pIBCard)
{
    //========================================================================
    //          
    //   see  http://cse.csusb.edu/tongyu/courses/cs520/notes/texture.php
    //   which in turn quotes Paul Bourke. 
    //              
    //              
    //      Create a sphere centered at sCentre, with radius sRadius,
    //      and precision nPrecision.
    //          
    //      Use sRadius = 1.6f with nPrecision = 24; 
    //              
    //========================================================================

    VHG_Spherolux                           tmpSpherolux{ };
    std::vector<VHG_Spherolux>              vectorSpherolux;
    std::vector<uint32_t>                   vectorIndexes;

    uint32_t                                idxV = 0; 

    uint32_t                                idxColat = 0;  //  Colatitude loop index; 
    uint32_t                                jdxLongit = 0; //  Longitude loop index;

    float                                   thetaColatitude = 0.f; // the present value of Colatitude;
    float                                   lambdaLongitude = 0.f; // the present value of Longitude;

    for (jdxLongit = 0; jdxLongit < tcs_precision; jdxLongit++)
    {
        for (idxColat = 0; idxColat < tcs_precision; idxColat++)  //  For Colatitude ranging from zero to pi: 
        {
            thetaColatitude = idxColat * DirectX::XM_PI / (float)tcs_precision;  //  Colatitude;

            lambdaLongitude = jdxLongit * DirectX::XM_2PI / (float)tcs_precision;   //  Longitude;
            tmpSpherolux = ComputeTextureCoordinates( thetaColatitude, lambdaLongitude, tcs_centre, tcs_radius, 0.f, 0.f );
            vectorSpherolux.push_back(tmpSpherolux); 

            lambdaLongitude = (1 + jdxLongit) * DirectX::XM_2PI / (float)tcs_precision;   //  Longitude;
            tmpSpherolux = ComputeTextureCoordinates( thetaColatitude, lambdaLongitude, tcs_centre, tcs_radius, 1.f, 0.f );
            vectorSpherolux.push_back(tmpSpherolux); 

            thetaColatitude = (1 + idxColat) * DirectX::XM_PI / (float)tcs_precision;  //  Colatitude; 


            lambdaLongitude = jdxLongit * DirectX::XM_2PI / (float)tcs_precision;   //  Longitude;
            tmpSpherolux = ComputeTextureCoordinates( thetaColatitude, lambdaLongitude, tcs_centre, tcs_radius, 0.f, 1.f );
            vectorSpherolux.push_back(tmpSpherolux); 

            lambdaLongitude = (1 + jdxLongit) * DirectX::XM_2PI / (float)tcs_precision;   //  Longitude;
            tmpSpherolux = ComputeTextureCoordinates( thetaColatitude, lambdaLongitude, tcs_centre, tcs_radius, 1.f, 1.f );
            vectorSpherolux.push_back(tmpSpherolux); 

            vectorIndexes.push_back(idxV); 
            vectorIndexes.push_back(idxV + 2); 
            vectorIndexes.push_back(idxV + 1); 

            vectorIndexes.push_back(idxV + 3); 
            vectorIndexes.push_back(idxV + 1); 
            vectorIndexes.push_back(idxV + 2); 

            idxV += 4;
        }
    }

    uint32_t vertex_count = (uint32_t)vectorSpherolux.size();
    *pVBCard = vertex_count;

    uint32_t index_count = (uint32_t)vectorIndexes.size();
    *pIBCard = index_count;




    std::vector<XMFLOAT3> plainPositionVertices(vertex_count);

    for (size_t i = 0; i < vertex_count; ++i)
    {
        plainPositionVertices[i] = vectorSpherolux[i].position;
    }



#define GHV_OPTION_PLAIN_POSITION_VERTICES


#ifdef GHV_OPTION_PLAIN_POSITION_VERTICES

    D3D11_SUBRESOURCE_DATA vertexBufferData = {0};
    vertexBufferData.pSysMem = &(plainPositionVertices[0]); 
    vertexBufferData.SysMemPitch = 0;
    vertexBufferData.SysMemSlicePitch = 0;

    CD3D11_BUFFER_DESC vertexBufferDesc(
        (UINT)vectorSpherolux.size() * sizeof(XMFLOAT3), 
        D3D11_BIND_VERTEX_BUFFER
    );

#else

    D3D11_SUBRESOURCE_DATA vertexBufferData = {0};
    vertexBufferData.pSysMem = &(vectorSpherolux[0]); 
    vertexBufferData.SysMemPitch = 0;
    vertexBufferData.SysMemSlicePitch = 0;

    CD3D11_BUFFER_DESC vertexBufferDesc(
        (UINT)vectorSpherolux.size() * sizeof(VHG_Spherolux), 
        D3D11_BIND_VERTEX_BUFFER
    );
#endif


    DX::ThrowIfFailed(
        tcs_deviceResources->GetD3DDevice()->CreateBuffer(
            &vertexBufferDesc, 
            &vertexBufferData, 
            tcs_vertex_buffer.GetAddressOf()
        )
    );


    D3D11_SUBRESOURCE_DATA indexBufferData = {0};
    indexBufferData.pSysMem = &(vectorIndexes[0]);
    indexBufferData.SysMemPitch = 0;
    indexBufferData.SysMemSlicePitch = 0;

    CD3D11_BUFFER_DESC indexBufferDesc( 
        (UINT)vectorIndexes.size() * sizeof(uint32_t), 
        D3D11_BIND_INDEX_BUFFER
    );
        
    DX::ThrowIfFailed(
        tcs_deviceResources->GetD3DDevice()->CreateBuffer(
            &indexBufferDesc, 
            &indexBufferData, 
            // ppIndexBuffer
            tcs_index_buffer.GetAddressOf()
        )
    );
}





#ifdef GHV_OPTION_EXTENDED_METHODS

void TexCoSphere::TexCoSphereRender(void)
{
    //      
    //      Render the sphere: 
    //


    auto context = m_deviceResources->GetD3DDeviceContext();


    UINT stride = sizeof(VHG_Spherolux);
    UINT offset = 0;
    context->IASetVertexBuffers( 0, 1, Sphere1_vertexBuffer.GetAddressOf(), &stride, &offset );

    //   Each entry of the Index Buffer is a uint32_t value, having size of 32 bits: 
    context->IASetIndexBuffer( Sphere1_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0 );

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


    context->IASetInputLayout(Sphere1_inputLayout.Get());


    context->VSSetShader(Sphere1_vertexShader.Get(), nullptr, 0 );

    context->VSSetConstantBuffers1( 0, 1, m_constantBuffer.GetAddressOf(), nullptr, nullptr );  // Slot zero;



    context->RSSetState(Sphere1_RasterizerState.Get());




    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


    // In this case const float gv_blend_factor[4] = { 1.f, 1.f, 1.f, 1.0f }; isn't used at all;


    context->OMSetBlendState(Sphere1_BlendState.Get(), NULL, 0xFFFFFFFF);  //  keywords: blendstate, alpha, blending;


    // This particular blendstate blending DOESN'T require changes in DeviceResources.cpp!!!


    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


    context->PSSetShader( Sphere1_pixelShader.Get(), nullptr, 0 );



#ifdef GHV_OPTION_SHOW_PRETTY_HYPERBOLIC
    context->PSSetShaderResources(0, 1, Sphere1_TextureSRV.GetAddressOf());  // Slot zero;
#else
    context->PSSetShaderResources(0, 1, Sphere1_DDS_A_SRV.GetAddressOf());  // Slot zero;
#endif

    context->PSSetSamplers(0, 1, Sphere1_TextureSamplerState.GetAddressOf());  // Slot zero;

    context->DrawIndexed( Sphere1_indexCount, 0, 0 );
}




void TexCoSphere::TexCoSphereCreateSamplerStateGeneric()
{

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //            Create a SamplerState for Pixel Shader
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));


    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;


    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;


    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;


    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateSamplerState(
            &sampDesc,
            Sphere1_TextureSamplerState.ReleaseAndGetAddressOf())
    );

}








void TexCoSphere::TexCoSphereDDSTextureFromFile(void)
{
    //                  
    //              DDS image properties: 
    //          ===============================
    //                  
    //  File  "asdf.dds" 
    //  originated as a full-color 923 x 923, 24 bpp, 96 dpi bmp.
    //              
    //  It was scaled down to 512 x 512 pixels, 24 bpp, 96 dpi bmp.
    //
    //  Then the color depth of the image
    //  was increased back to 24 bpp (using Increase Color Depth). 
    // 
    //  Thus a 512 x 512 pixel full color 24 bpp 96 dpi bmp. 
    //
    //  The bmp bitmap was then converted to dds in gimp using 
    //  dds image properties: 
    //          Format:             RGBA8,  
    //          Generate mipmaps:   10 mipmap levels.
    //                  


    Microsoft::WRL::ComPtr<ID3D11Resource> tmpResource;



    DX::ThrowIfFailed(
        DirectX::CreateDDSTextureFromFile(
            m_deviceResources->GetD3DDevice(),
            L"Assets\\a_square256.dds",
            tmpResource.ReleaseAndGetAddressOf(),
            Sphere1_DDS_A_SRV.ReleaseAndGetAddressOf(),
            0,
            nullptr
        )
    );



    DX::ThrowIfFailed(
        tmpResource.CopyTo(
            Sphere1_DDS_A_Texture.ReleaseAndGetAddressOf()
        )
    );
}



void TexCoSphere::TexCoSphereCreateRasterizerState(void)
{

    D3D11_RASTERIZER_DESC   rasterizer_description;
    ZeroMemory(&rasterizer_description, sizeof(rasterizer_description));

    rasterizer_description.MultisampleEnable = FALSE;

    // rasterizer_description.FillMode = D3D11_FILL_WIREFRAME; 
    rasterizer_description.FillMode = D3D11_FILL_SOLID;  //  SOLID;
    rasterizer_description.CullMode = D3D11_CULL_NONE;  //  Use CULL_NONE when BlendState makes sphere transparent;

    rasterizer_description.FrontCounterClockwise = true; // undo false;

    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //          
    //     Benefits of D3D11_CULL_NONE in the RASTERIZER:  
    //  =====================================================
    //  Suppose a 3D spinning cube is being rendered by D3D11. 
    //  And furthermore suppose the color has its alpha channel
    //  set low enough to make the cube transparent. 
    //  Then in order to present a compelling illusion of a 
    //  view into the interior of the semi-transparent cube
    //  it is NECESSARY to render triangles which otherwise
    //  might be culled. 
    //          
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



    DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateRasterizerState(
        &rasterizer_description,
        Sphere1_RasterizerState.ReleaseAndGetAddressOf()
    ));

}




void TexCoSphere::TexCoSphereCreateBlendState(void)
{
    //      
    //  ghv: The Sphere1_BlendState is applied to the texcosphere 
    //  while the sphere is being rendered, and this BlendState 
    //  allows one to see inside the texcosphere: the objects 
    //  inside the sphere become visible through the transparency
    //  of the sphere.
    //          


    D3D11_RENDER_TARGET_BLEND_DESC  rt_blend_descr = { 0 };

    rt_blend_descr.BlendEnable = TRUE;

    rt_blend_descr.SrcBlend = D3D11_BLEND_SRC_ALPHA;        // SrcBlend = D3D11_BLEND_SRC_ALPHA;
    rt_blend_descr.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;   // DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    rt_blend_descr.BlendOp = D3D11_BLEND_OP_ADD;            // BlendOp = D3D11_BLEND_OP_ADD;

    rt_blend_descr.SrcBlendAlpha = D3D11_BLEND_ONE;
    rt_blend_descr.DestBlendAlpha = D3D11_BLEND_ZERO;
    rt_blend_descr.BlendOpAlpha = D3D11_BLEND_OP_ADD;

    rt_blend_descr.RenderTargetWriteMask = 0x0F;





    D3D11_BLEND_DESC  d3d11_blend_descr = { 0 };

    d3d11_blend_descr.AlphaToCoverageEnable = TRUE; // Need AlphaToCoverageEnable = TRUE to make sphere transparent;


        // This is probably because the DDS image is of a multi-pane glass window fixture,
        // a matrix of glass with alpha = 0 ==> fully transparent 
        // separated by "wooden" cross members...which are opaque as wood often is.




    // d3d11_blend_descr.IndependentBlendEnable = FALSE;
    d3d11_blend_descr.IndependentBlendEnable = TRUE;



    d3d11_blend_descr.RenderTarget[0] = { rt_blend_descr };





    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBlendState(
            &d3d11_blend_descr,
            Sphere1_BlendState.GetAddressOf()
        )
    );
}

#endif



