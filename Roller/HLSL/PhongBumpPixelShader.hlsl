//          
//          
//  File PhongBumpPixelShader.hlsl    PS for the vehicle riding the track;
//  Garrett Vance 20170219
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//              
//          

Texture2D       tTexture_Mirror       : register(t0);
Texture2D       tTexture_Black        : register(t1);
Texture2D       tTexture_Normal       : register(t2);
TextureCube     tTexture_Cube         : register(t3);

SamplerState    colorSampler        : register(s0);
SamplerState    normalSampler       : register(s1);
SamplerState    cubeSampler         : register(s2);


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


float4 ps_main(PixelShaderInput input) : SV_TARGET
{
    float3 Ln = normalize(input.LightVec);
    float3 Vn = normalize(input.WorldView);
    float3 Nn = normalize(input.WorldNormal);
    float3 Tn = normalize(input.WorldTangent);
    float3 Bn = normalize(input.WorldBinormal);


    float BumpStrength = 3.1;   //  Use BumpStrength = 1.8;



    float SpecularExponent = 4.0;   // 1.0 to 128.0; 
    float3 LampColor = float3(1.0, 0.9, 1.0);  // usually white...
    float  Ks = 0.4; 
    float3  AmbientLightColor = float3(0.07, 0.07, 0.07); 
    float   Kr = 0.5; 




    float3 bump = BumpStrength * (tTexture_Normal.Sample(normalSampler, input.texco).rgb - float3(0.5, 0.5, 0.5));


    Nn = Nn + bump.x * Tn + bump.y * Bn;



    Nn = normalize(Nn);



    float3 Hn = normalize(Vn + Ln);



    float4 litV = lit(dot(Ln, Nn), dot(Hn, Nn), SpecularExponent);


    float3 diffContrib = litV.y * LampColor;


    float3 specContrib = litV.y * litV.z * Ks * LampColor;



    float3 diffuseColor = tTexture_Mirror.Sample(colorSampler, input.texco).rgb;

    if (input.colorCode > 1)
    {
        diffuseColor = tTexture_Black.Sample(colorSampler, input.texco).rgb;
    }


    float3 result = specContrib + (diffuseColor * (diffContrib + AmbientLightColor));


    float3 R = reflect(Vn, Nn);


    float3 reflColor = Kr * tTexture_Cube.Sample(cubeSampler, R.xyz).rgb;


    result += diffuseColor * reflColor;

    return float4(result, 1);


}
