#pragma Compute ToneMapping

#include "../../../src/host_device_shared/RenderOptions.h"


RWTexture2D<float4> SceneColor : register(u0, space0);
[[vk::push_constant]] ToneMappingOptions render_options;


float3 F(float3 x)
{
    const float A = 0.22f;
    const float B = 0.30f;
    const float C = 0.10f;
    const float D = 0.20f;
    const float E = 0.01f;
    const float F = 0.30f;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 Uncharted2ToneMapping(float3 color, float adapted_lum)
{
    const float WHITE = 11.2f;
    return F(1.6f * adapted_lum * color) / F(WHITE);
}

float3 ACESToneMapping(float3 color, float adapted_lum)
{
    const float A = 2.51f;
    const float B = 0.03f;
    const float C = 2.43f;
    const float D = 0.59f;
    const float E = 0.14f;
    color *= adapted_lum;
    return (color * (A * color + B)) / (color * (C * color + D) + E);
}


// ToneMapping and Gamma Correction
[numthreads(16, 16, 1)]
void ToneMapping(uint3 GlobalThreadID : SV_DispatchThreadID) {
    uint2 index = GlobalThreadID.xy;
    float3 color = SceneColor[index].xyz;

    if (render_options.EnableToneMapping) {
        color = ACESToneMapping(color, render_options.AdaptedLuminance);
    }
    if (render_options.EnableGammaCorrection) {
        const float gamma = 2.2f;
        color = pow(color, 1.f / gamma);
    }

    SceneColor[index] = float4(color, 1.f);
}
