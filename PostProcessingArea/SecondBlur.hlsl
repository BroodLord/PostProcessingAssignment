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

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{

	float softEdge = 0; // Softness of the edge of the circle - range 0.001 (hard edge) to 0.25 (very soft)
	float2 centreVector = input.areaUV - float2(0.5, 0.5f);
	float centreLengthSq = dot(centreVector, centreVector);
    
	float3 tc = float3(0.0, 0.0, 0.0);
    int Half = ((gblurStrength - 1) / 2 + 1);
    float3 ppColour = SceneTexture.Sample(PointSample, input.sceneUV) * gWeightArray[Half].x;
    
    float rt_w = 1 / gViewportHeight; // render target width
    float rt_h = 1 / gViewportWidth; // render target height
    int offset = 1;
    for (int i = Half - 1; i >= 0; i--)
    {
        tc += SceneTexture.Sample(PointSample, input.sceneUV + float2(rt_w * offset, 0.0f)) * gWeightArray[i].x +
        SceneTexture.Sample(PointSample, input.sceneUV - float2(rt_w * offset, 0.0f)) * gWeightArray[i].x;
        offset++;
    
    }
    offset = 1;
    float alpha = 1.0f - saturate((centreLengthSq - 0.25f + softEdge) / softEdge); // Soft circle calculation based on fact that this circle has a radius of 0.5 (as area UVs go from 0->1)
    ppColour += tc;
    return float4(ppColour, alpha);
 
}