
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

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    float4 finalColor = SceneTexture.Sample(PointSample, input.sceneUV.xy);
    float3 BasicColor = (0.1, 0.1, 0.1);
    
    //finalColor += SceneTexture.Sample(PointSample, input.sceneUV.xy);
    //finalColor += SceneTexture.Sample(PointSample, input.sceneUV.xy);
    //finalColor += SceneTexture.Sample(PointSample, input.sceneUV.xy);
    

    float Brightness = finalColor.r + finalColor.g + finalColor.b / 3;
    if (Brightness < 1.2f)
    {
        finalColor.r = 0.0f;
        finalColor.g = 0.0f;
        finalColor.b = 0.0f;
    }
    //finalColor.rgb += BasicColor;

    //float3 color = SceneTexture.Sample(PointSample, input.sceneUV.xy) * finalColor;
    return float4(finalColor.rgb, 1.0f);
}