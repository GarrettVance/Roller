//          
//          
//  File PTF_VertexShader.hlsl    VS for the lofted space curve;
//  Garrett Vance 20170219
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//              
//          

            
cbuffer WVP_ConstantBufferStruct : register(b0)
{
	matrix   model;
	matrix   view;
	matrix   projection;
    uint4    animator_count;
};


struct VertexShaderInput
{
	float3    pos       : POSITION;
	float2    texco     : TEXCOORD0; 
};


struct GEO_IN
{
    float4     pos         : SV_POSITION;
    float2     texco       : TEXCOORD0;
    float4     worldpos    : TEXCOORD1;
};



GEO_IN vs_main(VertexShaderInput input)
{
     GEO_IN output;
     
     float4 vertex_location = float4(input.pos, 1.0f);
     
     // Transform the vertex at "vertex_location" into projected space.
     
     vertex_location = mul(vertex_location, model);
     vertex_location = mul(vertex_location, view);
     vertex_location = mul(vertex_location, projection);
     output.pos = vertex_location;
     
     output.texco = input.texco;

     // output.worldpos = float4(input.pos, 1.f);

    //  Value of e_advert_modulus = 21 is set in class ctor.
    //  The cyclic values of animator_count 
    //  attains maximum value = (2 * e_advert_modulus).

    float taper = abs((float)animator_count - 21.f);
     
     output.worldpos = float4(
        input.pos.x + taper, 
        input.pos.y + taper,
        input.pos.z + taper,
        1.f);

     return output;
}




