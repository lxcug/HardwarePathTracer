#pragma once

#include "TraceUtils.hlsl"
#include "BRDF.hlsl"


struct MaterialSample {
    float3 direction;
    float pdf;

    float3 weight;  // brdf * cos / pdf
    float roughness;
};

MaterialSample SampleMaterial(in float3 ray_direction, in RayPayload payload, in float4 rnd) {
    MaterialSample mat_sample;
    static float specular_threshold = 1e-3f;
    static float roughness_threshold = .999f;
    float roughness = payload.roughness;
    mat_sample.roughness = roughness;
    if (roughness < specular_threshold) {
        mat_sample.direction = reflect(ray_direction, payload.normal);
        mat_sample.weight = payload.albedo * max(dot(payload.normal, mat_sample.direction), 0.f);
        mat_sample.pdf = 1.f;
    } else if (roughness < roughness_threshold) {
        const float min_f0 = 0.04f, max_f0 = 1.f;
        const float F0 = lerp(min_f0, max_f0, payload.metallic);
        float3 dir = SampleGGX(rnd.xy, -ray_direction, payload.normal, roughness);
        mat_sample.direction = dir;
        mat_sample.pdf = BRDF_PDF(-ray_direction, dir, payload.normal, roughness, F0);
        mat_sample.weight = payload.albedo *
        CookTorranceBRDF(-ray_direction, dir, payload.normal, roughness, F0) /
        mat_sample.pdf * max(dot(payload.normal, dir), 0.f);
    } else {
        float4 sample_value = CosineSampleHemisphere(rnd.xy, payload.normal);
        mat_sample.direction = sample_value.xyz;
        mat_sample.weight = payload.albedo;
        mat_sample.pdf = sample_value.w;
    }

    return mat_sample;
}

struct MaterialEval
{
	float3 weight;  // brdf * cos / pdf
	float pdf;
};

MaterialEval EvalMaterial(in float3 V, in float3 L, in RayPayload payload) {
    MaterialEval mat_eval;
    static float specular_threshold = 1e-3f;
    static float roughness_threshold = .999f;
    float roughness = payload.roughness;

    if (roughness < specular_threshold) {
        float3 H = normalize(V + L);
        if (dot(H, payload.normal) < SHADOWY_SMALL_NUMBER) {
            mat_eval.pdf = 1.f;
            mat_eval.weight = payload.albedo * max(dot(payload.normal, L), 0.f);
        }
    } else if (roughness < roughness_threshold) {
        const float min_f0 = 0.04f, max_f0 = 1.f;
        const float F0 = lerp(min_f0, max_f0, payload.metallic);
        mat_eval.pdf = BRDF_PDF(V, L, payload.normal, roughness, F0);
        mat_eval.weight = payload.albedo * CookTorranceBRDF(V, L, payload.normal, roughness, F0) *
        max(dot(payload.normal, L), 0.f) / mat_eval.pdf;
    } else {
        mat_eval.pdf = dot(payload.normal, L) / PI;
        mat_eval.weight = payload.albedo;
    }

    return mat_eval;
}
