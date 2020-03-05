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

float3 hsv(float h, float s, float v)
{
    return lerp(float3(1.0, 1.0, 1.0), clamp((abs(frac(
    h + float3(3.0, 2.0, 1.0) / 3.0) * 6.0 - 3.0) - 1.0), 0.0, 1.0), s) * v;
}

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    float4 col = float4(0.0, 0.0, 0.0, 0.0);
    float PSD = pow(abs(SceneTexture.Sample(PointSample, float2(0.5, 0.0)).r), 2.0);
    for (int i = 0; i < 100; i++)
    {
        // adapted from by iq https://www.shadertoy.com/view/MsKGWR
        float2 offset = gOffSet * cos(0.1 * float(i) + PSD + gITime + float2(0, .1));
        float4 t = SceneTexture.Sample(PointSample, input.sceneUV * .8 + offset + float2(.1, .0)) * 0.3;
        col += t * 5.;
    }
    
    col.rgb = hsv(col.x * .1 + gITime * .5 + PSD, 1., 1.);
    //float3 SceneColour = SceneTexture.Sample(PointSample, input.sceneUV) * col;
    return float4(col.rgb, 1.0f);
}