//          
//          
//  File SphyboxNon_PixelShader.hlsl    PS for small (inset) viewport;
//  Garrett Vance 20191103
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//              

TextureCube     sphybox_texcube;
SamplerState    sphybox_sampler : register(s0);

cbuffer conbufViewportInfo      : register(b0)
{
    float4  viewportDimension;
};

struct VertexOut
{
	float4 posClipSpace     : SV_Position;
    float3 posObjectSpace   : POSITION;
};

float4 ps_main(VertexOut psInput) : SV_Target
{
	float4 neutralColor = float4( 225/255.f, 195/255.f, 220/255.f, 1.f);
    float4 borderColor = float4(165 / 255.f, 54 / 255.f, 236 / 255.f, 1.f);

    //  Dimensions of the small viewport: 
    float dwX = viewportDimension.x;
    float dwY = viewportDimension.y;
    float dwW = viewportDimension.z;
    float dwH = viewportDimension.w;

    //  Use the dimensions of the small viewport 
    //  to transform from homogeneous clip space 
    //  into viewport space: 
    float4 Pvps = float4(
        psInput.posClipSpace.x * dwW,
        -psInput.posClipSpace.y * dwH,
        psInput.posClipSpace.z,
        psInput.posClipSpace.x * dwX + psInput.posClipSpace.y * (dwH + dwY) + psInput.posClipSpace.w
    );

    //  From the position in viewport space Pvps, 
    //  compute position in screen coords: 
    float Xs = Pvps.x / Pvps.w; 
    float Ys = Pvps.y / Pvps.w; 
    float Zs = Pvps.z / Pvps.w; 
    float Ws = 1.f / Pvps.w;

    //  Use the x & w coordinates of position in Projection Space, 
    //  i.e. the position immediately after multiplication by the Projection Matrix: 
    float projX = psInput.posClipSpace.x;
    float projY = psInput.posClipSpace.y;
    float projW = psInput.posClipSpace.w;

    float4 f4color = neutralColor;

    if( (projX <= 0.05f * dwW) || (projX >= 0.95f * dwW) || 
        (projY <= 0.05f * dwH) || (projY >= 0.95f * dwH) 
    )
    {
        f4color = borderColor;
    }

    return f4color;
}



