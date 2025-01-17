#pragma once

#include "TraceUtils.hlsl"


struct MaterialSample {
    float3 direction;
    float pdf;

    float3 weight;
    float roughness;
};

MaterialSample SampleMaterial(RayPayload payload, float4 rnd) {
    MaterialSample mat_sample;

    float4 sample_value = CosineSampleHemisphere(rnd.xy, payload.normal);
    mat_sample.direction = sample_value.xyz;
    mat_sample.pdf = sample_value.w;
    mat_sample.weight = payload.albedo;
    mat_sample.roughness = 1.f;

    return mat_sample;
}
