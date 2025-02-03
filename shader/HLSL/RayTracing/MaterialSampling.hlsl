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
    float metallic = payload.metallic;
    mat_sample.roughness = roughness;

    if (roughness < specular_threshold) {
        mat_sample.direction = reflect(ray_direction, payload.normal);
        mat_sample.weight = payload.albedo * max(dot(payload.normal, mat_sample.direction), 0.f);
        mat_sample.pdf = 1.f;
    } else {
        float3 V = -ray_direction;
        float3 F0 = lerp(float3(0.04, 0.04, 0.04), payload.albedo, payload.metallic);
        float3 F = Fresnel_Schlick(max(dot(payload.normal, V), 0.0), F0);

        float diffuse_lobe_selected_weight = LobeSelectionProb((1.0 - F) * (1.0 - metallic) * payload.albedo, payload.albedo);
        if (payload.metallic >= 1.0) {
            diffuse_lobe_selected_weight = 0.0;
        }

        float4 sample;
        if (rnd.w < diffuse_lobe_selected_weight) {
            sample = CosineSampleHemisphere(rnd.xy, payload.normal);
        } else {
            sample = SampleGGXReflection(rnd.xy, V, payload.normal, roughness);
        }
        float3 L = sample.xyz;
        mat_sample.direction = L;

        float NoL = max(dot(payload.normal, L), 0.f);
        float diffuse_pdf = max(NoL / PI, SHADOWY_SMALL_NUMBER);
        float3 diffuse_weight = payload.albedo;
        float specular_pdf = BRDF_PDF(V, L, payload.normal, roughness, F0);
        float3 specular_weight = CookTorranceBRDF(V, L, payload.normal, roughness, payload.metallic, payload.albedo, F0) * NoL / specular_pdf;

        mat_sample.pdf = 0.f;
        AddLobeWithMIS(mat_sample.weight, mat_sample.pdf, diffuse_weight, diffuse_pdf, diffuse_lobe_selected_weight);
        AddLobeWithMIS(mat_sample.weight, mat_sample.pdf, specular_weight, specular_pdf, 1.f - diffuse_lobe_selected_weight);
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
    float metallic = payload.metallic;

    if (roughness < specular_threshold) {
        float3 H = normalize(V + L);
        if (dot(H, payload.normal) < SHADOWY_SMALL_NUMBER) {
            mat_eval.pdf = 1.f;
            mat_eval.weight = payload.albedo * max(dot(payload.normal, L), 0.f);
        }
    } else {
        float3 F0 = lerp(float3(0.04, 0.04, 0.04), payload.albedo, payload.metallic);
        float NoL = max(dot(payload.normal, L), 0.f);

        float3 H = normalize(V + L);
        float HoV = max(dot(H, V), 0.f);
        float3 F = Fresnel_Schlick(HoV, F0);

        float diffuse_lobe_selected_weight = LobeSelectionProb((1.0 - F) * (1.0 - metallic) * payload.albedo, payload.albedo);
        if (payload.metallic >= 1.0) {
            diffuse_lobe_selected_weight = 0.0;
        }

        float diffuse_pdf = max(NoL / PI, SHADOWY_SMALL_NUMBER);
        float3 diffuse_weight = payload.albedo;
        float specular_pdf = BRDF_PDF(V, L, payload.normal, roughness, F0);
        float3 specular_weight = CookTorranceBRDF(V, L, payload.normal, roughness, payload.metallic, payload.albedo, F0) * NoL / specular_pdf;

        mat_eval.pdf = 0.f;
        AddLobeWithMIS(mat_eval.weight, mat_eval.pdf, diffuse_weight, diffuse_pdf, diffuse_lobe_selected_weight);
        AddLobeWithMIS(mat_eval.weight, mat_eval.pdf, specular_weight, specular_pdf, 1.f - diffuse_lobe_selected_weight);
    }

    return mat_eval;
}
