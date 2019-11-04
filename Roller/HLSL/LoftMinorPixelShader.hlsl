//          
//          
//  File LoftMinorPixelShader.hlsl    PS for the lofted space curve (inset viewport);
//  Garrett Vance 20170219
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//

Texture2D           trackTexture;
SamplerState        trackSampler;

struct PixelShaderInput
{
    float4     pos          : SV_POSITION;
    float2     texco        : TEXCOORD0;
    float4     color        : COLOR0;
};

float4       ps_main(PixelShaderInput      input) : SV_TARGET
{ 
    // float4 sampledTexture = trackTexture.Sample(trackSampler, input.texco); 

    return float4(0.1f, 0.1f, 0.1f, 1.f);
}




