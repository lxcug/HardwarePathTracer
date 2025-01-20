#pragma once

#include "Utils.hlsl"
#include "../../../src/host_device_shared/Light.h"


struct LightSample {
	float3 direction;
    float pdf;

	float3 radiance_over_pdf;
	float distance;
};


LightSample SampleDirectionalLight(in Light light, in float2 rnd, in float3 pos, in float3 normal) {
	float sin_theta_max = 0.0349;  // sin 2\degree  for soft shadows
	float4 dir_and_pdf = UniformSampleConeRobust(rnd, sin_theta_max * sin_theta_max);
    LightSample light_sample;
    light_sample.direction = LocalToWorld(-light.Direction, dir_and_pdf.xyz);
    light_sample.pdf = dir_and_pdf.w;
    light_sample.radiance_over_pdf = light.Color * light.Intensity;
    light_sample.distance = render_options.MaxTraceDistance;

    return light_sample;
}

LightSample SamplePointLight(in Light light, in float2 rnd, in float3 pos, in float3 normal) {
    float radius2 = light.Radius * light.Radius;
    float3 to_light = light.Position - pos;
    float distance_to_light2 = dot(to_light, to_light);
    float sin_theta_max2 = saturate(radius2 / distance_to_light2);
    float4 dir_and_pdf = UniformSampleConeRobust(rnd, sin_theta_max2);
    float cos_theta = dir_and_pdf.z;
    float sin_theta2 = 1.f - cos_theta * cos_theta;

    LightSample light_sample;
    light_sample.direction = LocalToWorld(normal, dir_and_pdf.xyz);
    light_sample.pdf = dir_and_pdf.w;
    float attenuation = PointLightAttenuation(light, distance_to_light2);

    light_sample.distance = length(to_light) * (cos_theta - sqrt(max(sin_theta_max2 - sin_theta2, 0.f)));

    float3 radiance = light.Color * light.Intensity / (PI * radius2);
    light_sample.radiance_over_pdf = radiance * attenuation / dir_and_pdf.w;

    return light_sample;
}

LightSample SampleLight(in Light light, in float2 rnd, in float3 pos, in float3 normal) {
    switch (light.Type) {
        case LightType::Directional:
            return SampleDirectionalLight(light, rnd, pos, normal);
        case LightType::Point:
            return SamplePointLight(light, rnd, pos, normal);
        default:
            return (LightSample)0;
    }
}

// Estimate Light PDF(Now intensity only)
float EstimateDirectionalLight(in Light light, in float3 pos, in float3 normal) {
    return Luminance(light.Color * light.Intensity);
}

float EstimatePointLight(in Light light, in float3 pos, in float3 normal) {
    float3 to_light = light.Position - pos;
    float dis2 = dot(to_light, to_light);
    float attenuation = PointLightAttenuation(light, dis2);
    float3 power = light.Color * light.Intensity / (PI * dis2);
    return Luminance(power * attenuation);
}

float EstimateLight(in Light light, in float3 pos, in float3 normal) {
    switch (light.Type) {
        case LightType::Directional:
            return EstimateDirectionalLight(light, pos, normal);
        case LightType::Point:
            return EstimatePointLight(light, pos, normal);
        default:
            return 0.f;
    }
}

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
