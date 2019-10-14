//          
//          
//  File PTF_PixelShader.hlsl    PS for the lofted space curve;
//  Garrett Vance 20170219
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//              
//          


struct GEO_IN
{
    float4     pos         : SV_POSITION;
    float2     texco       : TEXCOORD0;
    float4     worldpos    : TEXCOORD1;
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
    float  norm_squared = (input.worldpos.x * input.worldpos.x) 
    + (input.worldpos.y * input.worldpos.y) 
    + (input.worldpos.z * input.worldpos.z);

    float abs_radius = sqrt(norm_squared);

    float pct_x = abs(input.worldpos.x)/abs_radius;
    float pct_y = abs(input.worldpos.y)/abs_radius;
    float pct_z = abs(input.worldpos.z)/abs_radius;

    // float max_radius = 27.f; // too much red;

    float max_radius = 67.f;

    // max_radius = 122.f; // too much yellow;


    float relative_radius = (abs_radius < max_radius) ? abs_radius / max_radius : 1.f; 

    float hsl_lum = 0.5f; 
    float hsl_sat = 0.9f; 
    
    //  float4 v_rgba = color_HSL_to_RGB(relative_radius + 0.01f, hsl_sat, hsl_lum);

    return color_HSL_to_RGB(relative_radius + 0.01f, hsl_sat, hsl_lum);
}




