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

//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

float elapsedTime = 0.5;
float luminanceThreshold = 0.2;
float colorAmplification = 4.0;
float effectCoverage = 1.0;

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    float4 finalColor = SceneTexture.Sample(PointSample, input.sceneUV.xy);
    float3 BasicColor = (0.1, 0.1, 0.1);
    
    finalColor += SceneTexture.Sample(PointSample, input.sceneUV.xy);
    finalColor += SceneTexture.Sample(PointSample, input.sceneUV.xy);
    finalColor += SceneTexture.Sample(PointSample, input.sceneUV.xy);
    
    if (finalColor.r + finalColor.g + finalColor.b > 1.2f)
    {
        finalColor = finalColor / 4;
    }
    finalColor.rgb += BasicColor;
    finalColor.r = 0.0f;
    finalColor.g = ((finalColor.r + finalColor.g + finalColor.b) / 3);
    finalColor.b = 0.0f;
    float3 color = SceneTexture.Sample(PointSample, input.sceneUV.xy) * finalColor;
    return float4(color, 1.0f);

}
