//          
//          
//  File Sphybox_PixelShader.hlsl    PS for spherical skybox;
//  Garrett Vance 20170219
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//              
//          


TextureCube     sphybox_texcube;
SamplerState    sphybox_sampler : register(s0);


struct VertexOut
{
	float4 posClipSpace     : SV_Position;
    float3 posObjectSpace   : POSITION;
};
 


float4 ps_main(VertexOut psInput) : SV_Target
{
	return float4(1.f, 1.f, 1.f, 1.f);
}



