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
    float4 texColor = SceneTexture.Sample(PointSample, input.sceneUV);
    
 	// Initialize the variables
    float4 white = float4(1, 1, 1, 1);
    float4 black = float4(0, 0, 0, 1);
    
    // Average the color out
    float average = texColor.r + texColor.g + texColor.b / 3.0;
    
    // Check if it's closer to white or black
    if (average <= 0.5)
    {
        texColor = black;
    }
    else
        texColor = white;

  	// sets the pixels color
    //fragColor = texColor;
    return float4(texColor.rgb, 1.0f);
}