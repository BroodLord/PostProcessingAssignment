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


float random(float2 st)
{
    return frac(sin(dot(st.xy,
                         float2(12.9898, 78.233))) *
        43758.5453123);
}

float blend(const in float x, const in float y)
{
    return (x < 0.5) ? (2.0 * x * y) : (1.0 - 2.0 * (1.0 - x) * (1.0 - y));
}

float3 blend(const in float3 x, const in float3 y, const in float opacity)
{
    float3 z = float3(blend(x.r, y.r), blend(x.g, y.g), blend(x.b, y.b));
    return z * opacity + x * (1.0 - opacity);
}

float4 main(PostProcessingInput input) : SV_Target
{
   // float3 tc = float3(1.0, 0.0, 0.0);
   // float3 pixcol = SceneTexture.Sample(PointSample, input.sceneUV).rgb;
   // float3 colors[3];
   // colors[0] = float3(0., 0., 1.);
   // colors[1] = float3(1., 1., 0.);
   // colors[2] = float3(1., 0., 0.);
   // float lum = (pixcol.r + pixcol.g + pixcol.b) / 3.;
   // int ix = (lum < 0.5) ? 0 : 1;
   // tc = lerp(colors[ix], colors[ix + 1], (lum - float(ix) * 0.5) / 0.5);
   // 
   // return float4(tc, 1.0);
    
        
    float density = 1.6;
    float opacityScanline = .9;
    float opacityNoise = .9;
    float flickering = 0.01;
    
    float3 col = SceneTexture.Sample(PointSample, input.sceneUV).rgb;
    
    float count = gViewportHeight * density;
    float2 sl = float2(sin(input.sceneUV.y * count), cos(input.sceneUV.y * count));
    float3 scanlines = float3(sl.x, sl.y, sl.x);

    col += col * scanlines * opacityScanline;
    col += col * float3(random(input.sceneUV * gHueLevel), random(input.sceneUV * gHueLevel), random(input.sceneUV * gHueLevel)) * opacityNoise;
    col += col * sin(110.0 * gHueLevel) * flickering;
    float grey = (col.r + col.g + col.b) / 3.0f;


    return float4(grey, grey, grey, 1.0);
  

}
