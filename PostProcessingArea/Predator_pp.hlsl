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
    float3 pixcol = SceneTexture.Sample(PointSample, input.sceneUV).rgb;
    float3 colors[3];
    colors[0] = float3(0., 0., 1.);
    colors[1] = float3(1., 1., 0.);
    colors[2] = float3(1., 0., 0.);
    float lum = (pixcol.r + pixcol.g + pixcol.b) / 3.;
    int ix = (lum < 0.5) ? 0 : 1;
    tc = lerp(colors[ix], colors[ix + 1], (lum - float(ix) * 0.5) / 0.5);

    return float4(tc, 1.0);
}