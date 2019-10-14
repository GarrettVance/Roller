//          
//          
//  File PhongBumpVertexShader.hlsl    VS for the vehicle riding the track;
//  Garrett Vance 20170219
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//              
//          
//               



#undef FLIP_TEXTURE_Y	 //  Different in OpenGL & DirectX 




#define LIGHT_COORDS "World"  //  Lights and lamps specified in World coordinates rather than Model coords;




static float  w_component = 1;  //  w_component = 1 makes LampDirPos a point's position on the w = 1 plane; 

//  static float  w_component = 0;  //  w_component = 0 makes LampDirPos a direction vector; 



// static float4 LampDirPos = float4(-2.f, +6.f, 5.f, w_component);  //  LampDirPos can be either position or direction depending on w_component; 

static float4 LampDirPos = float4(-2.f, +2.f, 1.f, w_component);  //  LampDirPos can be either position or direction depending on w_component; 



cbuffer MVPConstantBuffer : register(b0)
{
    float4x4    trxWVP;  
    float4x4    trxWorld;
    float4x4    trxInvTposeWorld;
    float4x4    trxInverseView;
    uint4       e_animator_counts;
};




struct VertexShaderInput
{
	float3 pos          : POSITION;
	float3 normal       : NORMAL;
	float2 texco        : TEXCOORD;
    float3 tangent      : TANGENT; 
    float3 bitangent    : BITANGENT; 
};




struct PixelShaderInput
{
	float4  HPosition           : SV_POSITION;
	float2  texco               : TEXCOORD0;

    // The following values are passed in "World" coordinates:  

    float3  LightVec            : TEXCOORD1;
    float3  WorldNormal         : TEXCOORD2;
    float3  WorldTangent        : TEXCOORD3;
    float3  WorldBinormal       : TEXCOORD4;
    float3  WorldView           : TEXCOORD5;
    int     colorCode : TEXCOORD6;
};




PixelShaderInput vs_main(VertexShaderInput input)
{
	PixelShaderInput output;

    float4 inNormal = float4(input.normal, 1.0f);
    float3 nWorld = mul(inNormal, trxInvTposeWorld).xyz; // the Normal in World Coords; 
    // redundant mul : output.WorldNormal = mul(inNormal, trxInvTposeWorld).xyz; 
    output.WorldNormal = nWorld;  


    float4 inTangent = float4(input.tangent, 1.0f); 
    output.WorldTangent = mul(inTangent, trxInvTposeWorld).xyz;


    float4 inBitangent = float4(input.bitangent, 1.0f);
    output.WorldBinormal = mul(inBitangent, trxInvTposeWorld).xyz;  // TODO: clean up terminology binormal vs bitangent!!!


    float4 Po = float4(input.pos.xyz, 1);  //  "Po" is position in object-space coords;


    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //      
    //  My code to test whether we are on one of the horizontal faces
    //  of the space pod actually works as intended. 
    //  But my wavefront obj mesh model has some irregularities 
    //  which detract from the brilliance of my dot product idea...
    //      
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    float4 upwardDirection = float4(0.0f, 1.0f, 0.0f, 1.0f);
    float3 vWorld = mul(upwardDirection, trxInvTposeWorld).xyz;  //  the vertical direction in World Coords; 


    float f1 = length(nWorld) * length(vWorld); 
    float f2 = abs(dot(nWorld, vWorld)); 

    if (abs(f1 - f2) < 0.0001)
    {
        //  The normal vector is (mostly) parallel to the world's vertical direction
        //  meaning this is either the upper or lower face of the pod, 

        output.colorCode = 0; // use the mirror texture: 
    }
    else
    {
        output.colorCode = 2; // use dark texture: 
    }
    
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$


    float4 Pw = mul(Po, trxWorld);	// convert "Po" from object-space to "world" space to get position in world coords; 


    float4 Lw = LampDirPos;  //  Lamp in world coords (per #define of LIGHT_COORDS at top of file);
     


    if (Lw.w == 0) 
    {
        //   w_component == 0 implies that Lw is a direction vector: 

        output.LightVec = -normalize(Lw.xyz);
    }
    else 
    {
        //  w_component is 1, meaning that Lw is a Point Position. 
        //  Obtain direction vector from Point Position by subtraction: 

        // we are still passing a (non-normalized) vector

        output.LightVec = Lw.xyz - Pw.xyz;
    }





#ifdef FLIP_TEXTURE_Y

    output.texco = float2(input.texco.x, (1.0 - input.texco.y));

#else /* !FLIP_TEXTURE_Y */

    output.texco = input.texco.xy;

#endif /* !FLIP_TEXTURE_Y */




    output.WorldView = normalize(trxInverseView[3].xyz - Pw.xyz);  // TODO: what is 3rd column of View Inverse matrix ??? 



    output.HPosition = mul(Po, trxWVP);  //  "Po" is position in object-space coords;

    
    return output;
}



