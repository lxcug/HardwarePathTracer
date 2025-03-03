#pragma once

#include "Utils.hlsl"
#include "../../../src/host_device_shared/Light.h"


float EstimateDirectionalLight(in Light light, in float3 pos, in float3 normal) {
    return Luminance(light.Color * light.Intensity);
}

float EstimatePointLight(in Light light, in float3 pos, in float3 normal) {
    float3 to_light = light.Position - pos;
    float dis2 = dot(to_light, to_light);
    float attenuation = PointLightAttenuation(light, dis2);
    return Luminance(light.Color * light.Intensity * attenuation);
}

float EstimateSkyLight(in Light light, in float3 pos, in float3 normal) {
    return 4 * PI * light.Intensity;
}

// Estimate Light Pick PDF(power based)
float EstimateLight(in Light light, in float3 pos, in float3 normal) {
    switch (light.Type) {
        case LightType::Directional:
            return EstimateDirectionalLight(light, pos, normal);
        case LightType::Point:
            return EstimatePointLight(light, pos, normal);
        case LightType::Sky:
            return EstimateSkyLight(light, pos, normal);
        default:
            return 0.f;
    }
}

// Select a light via CDF
void SelectLight(in float rnd, inout float light_pick_cdf[64], out uint light_index, out float light_pick_pdf) {
    light_index = 0;
    for (int count = render_options.NumLights; count > 0;) {
        int step = count / 2;
        int iterator = light_index + step;
        if (rnd < light_pick_cdf[iterator]) {
            count = step;
        } else {
            light_index = iterator + 1;
            count -= step + 1;
        }
    }

    light_pick_pdf = light_pick_cdf[light_index] - (light_index > 0 ? light_pick_cdf[light_index - 1] : 0.f);
}

struct LightSample {
	float3 direction;
    float pdf;

	float3 radiance_over_pdf;
	float distance;
};

LightSample SampleDirectionalLight(in Light light, in float2 rnd, in float3 pos, in float3 normal) {
	float sin_theta_max = light.HalfSinAngleOrRange;
	float4 dir_and_pdf = UniformSampleConeRobust(rnd, sin_theta_max * sin_theta_max);
    LightSample light_sample;
    light_sample.direction = LocalToWorld(-light.Direction, dir_and_pdf.xyz);
    light_sample.pdf = dir_and_pdf.w;
    /*
     * NOTE: For Directional Light, Lo / pdf = light.Color * light.Intensity
     */
    light_sample.radiance_over_pdf = light.Color * light.Intensity;
    light_sample.distance = render_options.MaxTraceDistance;

    return light_sample;
}

LightSample SamplePointLight(in Light light, in float2 rnd, in float3 pos, in float3 normal) {
    float radius = max(light.Radius, SHADOWY_SMALL_NUMBER);
    float radius2 = pow(radius, 2);
    float3 to_light = light.Position - pos;
    float dis2 = dot(to_light, to_light);
    float sin_theta_max2 = saturate(radius2 / dis2);
    float4 dir_and_pdf = UniformSampleConeRobust(rnd, sin_theta_max2);
    float cos_theta = dir_and_pdf.z;
    float sin_theta2 = 1.f - cos_theta * cos_theta;

    LightSample light_sample;
    light_sample.direction = LocalToWorld(normalize(to_light), dir_and_pdf.xyz);
    light_sample.pdf = dir_and_pdf.w;
    float attenuation = PointLightAttenuation(light, dis2);

    light_sample.distance = length(to_light) * (cos_theta - sqrt(max(sin_theta_max2 - sin_theta2, 0.f)));

    // Actually / (4 * PI * radius2), 4 is missing for some reason(align with ue)
    float3 radiance = light.Color * light.Intensity / (PI * radius2);
    light_sample.radiance_over_pdf = radiance * attenuation / light_sample.pdf;

    return light_sample;
}

LightSample SampleSkyLight(in Light light, in float2 rnd, in float3 pos, in float3 normal) {
    LightSample light_sample;
    float4 sample_value = UniformSampleSphere(rnd);
    light_sample.direction = sample_value.xyz;
    light_sample.pdf = sample_value.w;
    light_sample.radiance_over_pdf = light.Intensity *
                                     min(SkyTexture.SampleLevel(SkyTextureSampler, uint_vector_to_hdri_uv(-light_sample.direction), 0).rgb, 1e2f) /
                                     light_sample.pdf;
    light_sample.distance = render_options.MaxTraceDistance;
    return light_sample;
}

/*
 * Sample Light's Solid Angle
 */
LightSample SampleLight(in Light light, in float2 rnd, in float3 pos, in float3 normal) {
    switch (light.Type) {
        case LightType::Directional:
            return SampleDirectionalLight(light, rnd, pos, normal);
        case LightType::Point:
            return SamplePointLight(light, rnd, pos, normal);
        case LightType::Sky:
            return SampleSkyLight(light, rnd, pos, normal);
        default:
            return (LightSample)0;
    }
}
