//          
//          
//  File Sphybox_VertexShader.hlsl    VS for spherical skybox;
//  Garrett Vance 20170219
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//              
//          


cbuffer conbufMVP
{
	float4x4    WorldViewProj;
};


struct VertexIn
{
	float3 posObjectSpace : POSITION;
};


struct VertexOut
{
	float4 posClipSpace     : SV_Position;
    float3 posObjectSpace   : POSITION;
};
 

VertexOut vs_main(VertexIn vin)
{
	VertexOut vout;

    float4 position4L = float4(vin.posObjectSpace, 1.0f); 
	
    //      
    //          x y w w swizzle
    //  =================================================
    //  Note carefully the "xyww" swizzle appended to 
    //  the matrix multiplication: It takes the w-component 
    //  of the matrix product and writes it into the position 
    //  reserved for the z-component. In other words, 
    //  the swizzle sets z equal to w.  
    //      
    //  Then z/w = 1 identically. The vertex shader emits 
    //  SV_POSITION with depth = 1. 
    //      

	vout.posClipSpace = mul(position4L, WorldViewProj).xyww;
	

	vout.posObjectSpace = vin.posObjectSpace; // pass-through;
	
	return vout;
}







