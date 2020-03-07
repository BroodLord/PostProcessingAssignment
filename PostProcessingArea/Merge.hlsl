
#include "Common.hlsli"

//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// The scene has been rendered to a texture, these variables allow access to that texture
Texture2D SceneTexture : register(t0);
SamplerState PointSample : register(s0); // We don't usually want to filter (bilinear, trilinear etc.) the scene texture when
                                          // post-processing so this sampler will use "point sampling" - no filtering

Texture2D SceneTexture1 : register(t1);



//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    float3 Sample1 = SceneTexture.Sample(PointSample, input.sceneUV);
    float3 Sample2 = SceneTexture1.Sample(PointSample, input.sceneUV);
    float3 MergedSample = Sample1 + (Sample2 * 2.5);
    return float4(MergedSample, 1.0f);

}