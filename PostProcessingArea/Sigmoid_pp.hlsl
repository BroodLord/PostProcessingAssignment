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

float Cubic

    (

    float value)
{
	
    // Possibly slightly faster calculation
    // when compared to Sigmoid
    
    if (value < 0.5)
    {
        return value * value * value * value * value * 16.0;
    }
    
    value -= 1.0;
    
    return value * value * value * value * value * 16.0 + 1.0;
}

float Sigmoid

    (

    float x)
{

	//return 1.0 / (1.0 + (exp(-(x * 14.0 - 7.0))));
    return 1.0 / (1.0 + (exp(-(x - 0.5) * 14.0)));
}

float4 main(PostProcessingInput input) : SV_Target
{
    
    float gamma = gGamma;
    
    //float2 uv = input.sceneUV.xy / gViewportWidth + gViewportHeight;
	
    float4 C = SceneTexture.Sample(PointSample, input.sceneUV);
    float4 A = C;
    
    C = float4(Cubic(C.r), Cubic(C.g), Cubic(C.b), 1.0);
    	//C = vec4(Sigmoid(C.r), Sigmoid(C.g),Sigmoid(C.b), 1.0); 
        
    C = pow(C, float(gamma));
    float3 V = SceneTexture.Sample(PointSample, input.sceneUV) * C;
    
    return float4(V, 1.0f);
}