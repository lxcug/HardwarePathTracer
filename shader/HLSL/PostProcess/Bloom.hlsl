#pragma Compute BrightnessExtract
#pragma Compute HorizontalBlur
#pragma Compute VerticalBlur
#pragma Compute Composite


#include "../../../src/host_device_shared/RenderOptions.h"
#include "../../../src/host_device_shared/ViewUniformBuffer.h"

RWTexture2D<float4> SceneColor : register(u0, space0);
RWTexture2D<float4> TempBrightness : register(u1, space0);

#define _BloomThreshold 1
#define _BlurRadius 2
#define _Sigma 1.5
#define _Stride 1

float GaussianWeight(float2 xy, float sigma) {
    float sigma2 = pow(sigma, 2);
    float a = -(dot(xy, xy)) / (2 * sigma2);
    return pow(M_E, a) / (2.f * PI * sigma2);
}

[numthreads(16, 16, 1)]
void BrightnessExtract(uint3 GlobalThreadID : SV_DispatchThreadID) {
    uint2 index = GlobalThreadID.xy;
    float3 color = SceneColor[index].rgb;
    float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
    float bright = saturate((luminance - _BloomThreshold));
    TempBrightness[index] = float4(color * bright, 1.0);
}

[numthreads(16, 16, 1)]
void HorizontalBlur(uint3 GlobalThreadID : SV_DispatchThreadID) {
    uint2 index = GlobalThreadID.xy;
    float3 sum = 0;
    float weight_sum = 0;
    for (int i = -_BlurRadius; i <= _BlurRadius; i++) {
        float weight = GaussianWeight(float2(i * _Stride, 0), _Sigma);
        sum += TempBrightness[index + int2(i * _Stride, 0)].rgb * weight;
        weight_sum += weight;
    }

    TempBrightness[index] = float4(sum / weight_sum, 1.0);
}

[numthreads(16, 16, 1)]
void VerticalBlur(uint3 GlobalThreadID : SV_DispatchThreadID) {
    uint2 index = GlobalThreadID.xy;
    float3 sum = 0;
    float weight_sum = 0;
    for (int i = -_BlurRadius; i <= _BlurRadius; i++) {
        float weight = GaussianWeight(float2(i * _Stride, 0), _Sigma);
        sum += TempBrightness[index + int2(0, i * _Stride)].rgb * weight;
        weight_sum += weight;
    }

    TempBrightness[index] = float4(sum / weight_sum, 1.0);
}

[numthreads(16, 16, 1)]
void Composite(uint3 GlobalThreadID : SV_DispatchThreadID) {
    uint2 index = GlobalThreadID.xy;

    float3 scene_color = SceneColor[index].rgb;
    float3 bloom_color = TempBrightness[index].rgb;

    SceneColor[index] = float4(scene_color + bloom_color, 1.0);
}
