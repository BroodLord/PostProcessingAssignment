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

	float offset[] = { 0, 1, 2, 3, 4 };
	float weight[] = { 0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162 };
	float3 tc = float3(0.0, 0.0, 0.0);
	float3 ppColour = SceneTexture.Sample(PointSample, input.sceneUV) * weight[0];
    
    float rt_w = 1 / gViewportHeight * gblurStrength; // render target width
    float rt_h = 1 / gViewportWidth * gblurStrength; // render target height
   
    for (int i = 1; i < 5; i++)
    {
        tc += SceneTexture.Sample(PointSample, input.sceneUV + float2(rt_w * i, 0.0f)) * weight[i] +
        SceneTexture.Sample(PointSample, input.sceneUV - float2(rt_w * i, 0.0f)) * weight[i];
    }
   
    float alpha = 1.0f - saturate((centreLengthSq - 0.25f + softEdge) / softEdge); // Soft circle calculation based on fact that this circle has a radius of 0.5 (as area UVs go from 0->1)
    ppColour += tc;
    return float4(ppColour, alpha);
    
 
}