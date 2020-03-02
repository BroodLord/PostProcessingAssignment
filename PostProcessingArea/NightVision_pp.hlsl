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

float elapsedTime = 0.5;
float luminanceThreshold = 0.2;
float colorAmplification = 4.0;
float effectCoverage = 1.0;

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
	float4 finalColor;
   //Set effectCoverage to 1.0 for normal use.  
    float2 uv;           
    uv.x = 0.4*sin(elapsedTime*50.0);                                 
    uv.y = 0.4*cos(elapsedTime*50.0);           
   //float2 noiseUV = input.sceneUV * gNoiseScale + gNoiseOffset;
	float m = (NoiseMap, input.sceneUV.xy).r;
	float3 n = SceneTexture.Sample(TrilinearWrap, input.sceneUV.xy + uv).rgb;
	float3 c = SceneTexture.Sample(PointSample, input.sceneUV.xy + (n.xy * 0.005)).rgb;
  
    float lum = dot(float3(0.30, 0.59, 0.11), c);
    if (lum < luminanceThreshold)
      c *= colorAmplification; 
  
	float3 visionColor = float3(0.1, 0.95, 0.2);
    finalColor.rgb = (c + (n*0.2)) * visionColor * m;
   //}
   //else
   //{
	//	finalColor = SceneTexture.Sample(PointSample,
   //                input.sceneUV[0]);
   //}
	float3 Colour = SceneTexture.Sample(PointSample, input.sceneUV).rbg * visionColor.rgb; //.rgb;
	return float4(Colour, 1.0f);
}
