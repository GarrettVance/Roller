//          
//          
//  File PTF_PixelShader.hlsl    PS for the lofted space curve;
//  Garrett Vance 20170219
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//              
//          

Texture2D           trackTexture;
SamplerState        trackSampler;


struct GEO_IN
{
    float4     pos          : SV_POSITION;
    float2     texco        : TEXCOORD0;
    float4     color        : COLOR0;
};



float sub_hue_to_rgb(float p, float q, float t)
{
    float retval = 0.f; 
    float tt = t;
    if (tt < 0.0) tt += 1.0;
    if (tt > 1.0) tt -= 1.0;
  
    if (tt < 1.0/6.0) retval = p + (q - p) * 6.0 * tt;
    else if (tt < 1.0/2.0) retval = q;
    else if (tt < 2.0/3.0) retval = p + (q - p) * (2.0/3.0 - tt) * 6.0;
    else retval = p;

    return retval;
}




float4 color_HSL_to_RGB(float h, float s, float l)
{
    float r = 0.f;
    float g = 0.f;
    float b = 0.f;

    if(s == 0.0)
    {
        r = g = b = l; // achromatic
    }
    else
    {
        float q;
    
        if (l < 0.5)
        { 
            q = l * (1.0 + s);
        } 
        else
        { 
            q = l + s - l * s;
        }
     
        float p = 2.0 * l - q;
    
        r = sub_hue_to_rgb(p, q, h + 1.0/3.0);
    
        g = sub_hue_to_rgb(p, q, h);
    
        b = sub_hue_to_rgb(p, q, h - 1.0/3.0);
    }
  
    return float4(r, g, b, 1.f);
}



float4       ps_main(GEO_IN      input) : SV_TARGET
{ 
    // float hsl_lum = 0.5f; 
    // float hsl_sat = 0.9f; 

    float4 sampledTexture = trackTexture.Sample(trackSampler, input.texco); 

    return input.color * sampledTexture;
}




