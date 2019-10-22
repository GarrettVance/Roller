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
	float3    pos           : POSITION;
	float2    texco         : TEXCOORD0; 
    uint      segment_id    : TEXCOORD1; 
};


struct GEO_IN
{
    float4     pos          : SV_POSITION;
    float2     texco        : TEXCOORD0;
    float4     color        : COLOR0;
};





static const float4 allColors[] = 
{
    float4(0.0f, 0.0f, 0.0f, 1.0f),
    float4(0.5f, 0.0f, 0.0f, 1.0f),
    float4(1.0f, 0.0f, 0.0f, 1.0f),

    float4(0.0f, 0.5f, 0.0f, 1.0f),
    float4(0.5f, 0.5f, 0.0f, 1.0f),
    float4(1.0f, 0.5f, 0.0f, 1.0f),

    float4(0.0f, 1.0f, 0.0f, 1.0f),
    float4(0.5f, 1.0f, 0.0f, 1.0f),
    float4(1.0f, 1.0f, 0.0f, 1.0f),



    float4(0.0f, 0.0f, 0.5f, 1.0f),
    float4(0.5f, 0.0f, 0.5f, 1.0f),
    float4(1.0f, 0.0f, 0.5f, 1.0f),

    float4(0.0f, 0.5f, 0.5f, 1.0f),
    float4(0.5f, 0.5f, 0.5f, 1.0f),
    float4(1.0f, 0.5f, 0.5f, 1.0f),

    float4(0.0f, 1.0f, 0.5f, 1.0f),
    float4(0.5f, 1.0f, 0.5f, 1.0f),
    float4(1.0f, 1.0f, 0.5f, 1.0f),

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

     uint idxColor = input.segment_id % 9; 

     output.color = allColors[idxColor]; 

     return output;
}




