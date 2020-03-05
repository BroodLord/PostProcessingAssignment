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

const float eps = 0.005;
const float far = 20.;

float map(float3 p)
{
    p.z -= gITime;
    float3 m;
    modf(p, m);
    return length(m - 1.) - .5;
}

float3 calcNormal(float3 p)
{
    float2 e = float2(eps, 0);
    return normalize(float3(map(p + e.xyy) - map(p - e.xyy), map(p + e.yxy) - map(p - e.yxy), map(p + e.yyx) - map(p - e.yyx)));
}

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    float3 ro = 0;
    float3(input.sceneUV, 1.);
    float3 rd = normalize(float3(input.sceneUV, -1.));
    float t = 0.;
    for (int i = 0; i < 100; i++)
    {
        float m = map(ro + rd * t);
        t += m;
        if (m < eps || t > far)
            break;
    }
 
    float3 p = ro + rd * t;
    float3 n = calcNormal(p);
    float3 lp = float3(1., 4., 5.);
    float3 ld = lp - p;
    float len = length(ld);
    ld /= len;
    float diff = max(dot(ld, n), 0.);
    float3 col = float3(1.0, 1.0, 1.0) * diff;
    return float4(col, 1.0);
}