//--------------------------------------------------------------------------------------
// Colour Tint Post-Processing Pixel Shader
//--------------------------------------------------------------------------------------
// Just samples a pixel from the scene texture and multiplies it by a fixed colour to tint the scene

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// The scene has been rendered to a texture, these variables allow access to that texture
Texture2D SceneTexture : register(t0);
SamplerState PointSample : register(s0); // We don't usually want to filter (bilinear, trilinear etc.) the scene texture when
										  // post-processing so this sampler will use "point sampling" - no filtering
// This shader also uses a texture filled with noise
Texture2D NoiseMap : register(t1);
SamplerState TrilinearWrap : register(s1);

//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

float4 main(PostProcessingInput input) : SV_Target
{
	  
    float3 tc = float3(1.0, 0.0, 0.0);

    float dx = 15.0f * (1. / gViewportWidth);
    float dy = 10.0f * (1. / gViewportHeight);
    float2 coord = float2(dx * floor(input.sceneUV.x / dx), dy * floor(input.sceneUV.y / dy));
    tc = SceneTexture.Sample(PointSample, coord).rgb;
    tc.r *= 20;
    tc.g *= 20;
    tc.b *= 20;
    int TCR = (int)tc.r;
    int TCG = (int) tc.g;
    int TCB = (int) tc.b;
    float3 NewTC;
    NewTC.r = TCR / 20.0f;
    NewTC.g = TCG / 20.0f;
    NewTC.b = TCB / 20.0f;
    //float3 NewColour = SceneTexture.Sample(PointSample, coord).rgb;
    
    return float4(NewTC, 1.0f);
}