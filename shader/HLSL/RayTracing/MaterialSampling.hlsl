#pragma once

#include "BRDF.hlsl"


#define DIFFUSE_ONLY 0


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

#if DIFFUSE_ONLY
    float4 sample = CosineSampleHemisphere(rnd.xy, payload.normal);
    mat_sample.direction = sample.xyz;
    mat_sample.weight = payload.albedo;
    mat_sample.pdf = sample.w;
#else
    if (roughness < specular_threshold) {
        float3 V = -ray_direction;
        float3 L = reflect(ray_direction, payload.normal);
        mat_sample.direction = L;

        float3 F0 = lerp(float3(0.04, 0.04, 0.04), payload.specular, metallic);
        float NoL = max(dot(payload.normal, L), 0.f);

        float3 H = normalize(V + L);
        float HoV = max(dot(H, V), 0.f);
        float3 F = Fresnel_Schlick(HoV, F0, roughness);

        float diffuse_lobe_selected_weight = LobeSelectionProb(payload.albedo, payload.specular);

        float NoV = max(dot(payload.normal, V), 0.f);
        float HoL = max(dot(H, L), 0.f);

        float diffuse_pdf = max(NoL / PI, SHADOWY_SMALL_NUMBER);
        float3 diffuse_weight = Diffuse_Burley_Disney(payload.albedo, F0, roughness, NoV, NoL, HoL) * NoL / diffuse_pdf;

        float specular_pdf = BRDF_PDF(V, L, payload.normal, roughness, F0);
        float3 specular_weight = CookTorranceBRDF(V, L, payload.normal, roughness, metallic, payload.albedo, F0) * NoL / specular_pdf;

        mat_sample.pdf = 0.f;
        mat_sample.weight = 0.f;
        float3 Kd = (1 - F) * (1 - metallic);
        AddLobeWithMIS(mat_sample.weight, mat_sample.pdf, Kd * diffuse_weight, diffuse_pdf, diffuse_lobe_selected_weight);
        AddLobeWithMIS(mat_sample.weight, mat_sample.pdf, F * max(dot(payload.normal, L), 0.f), 1.f, 1.f - diffuse_lobe_selected_weight);
    } else {
        float3 V = -ray_direction;
        float3 F0 = lerp(float3(0.04, 0.04, 0.04), payload.specular, metallic);
        float3 F = Fresnel_Schlick(max(dot(payload.normal, V), 0.0), F0, roughness);

        float diffuse_lobe_selected_weight = LobeSelectionProb(payload.albedo, payload.specular);

        float4 sample;
        if (rnd.w < diffuse_lobe_selected_weight) {
            sample = CosineSampleHemisphere(rnd.xy, payload.normal);
        } else {
            sample = SampleGGXReflection(rnd.xy, V, payload.normal, roughness);
        }

        float3 L = sample.xyz;
        mat_sample.direction = L;

        float NoL = max(dot(payload.normal, L), 0.f);

        float3 H = normalize(V + L);
        float HoV = max(dot(H, V), 0.f);
        float NoV = max(dot(payload.normal, V), 0.f);
        float HoL = max(dot(H, L), 0.f);

        float diffuse_pdf = max(NoL / PI, SHADOWY_SMALL_NUMBER);
        float3 diffuse_weight = Diffuse_Burley_Disney(payload.albedo, F0, roughness, NoV, NoL, HoL) * NoL / diffuse_pdf;

        float specular_pdf = BRDF_PDF(V, L, payload.normal, roughness, F0);
        float3 specular_weight = CookTorranceBRDF(V, L, payload.normal, roughness, metallic, payload.albedo, F0) * NoL / specular_pdf;

        mat_sample.pdf = 0.f;
        mat_sample.weight = 0.f;
        float3 Kd = (1 - F) * (1 - metallic);
        AddLobeWithMIS(mat_sample.weight, mat_sample.pdf, Kd * diffuse_weight, diffuse_pdf, diffuse_lobe_selected_weight);
        AddLobeWithMIS(mat_sample.weight, mat_sample.pdf, specular_weight, specular_pdf, 1.f - diffuse_lobe_selected_weight);
    }
#endif

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

#if DIFFUSE_ONLY
    float NoL = max(dot(payload.normal, L), 0.f);
    mat_eval.weight = payload.albedo;
    mat_eval.pdf = max(NoL / PI, SHADOWY_SMALL_NUMBER);
#else
    if (roughness < specular_threshold) {
        float3 H = normalize(V + L);
        mat_eval.pdf = 1.f;
        mat_eval.weight = 0.f;
        if (dot(H, payload.normal) < SHADOWY_SMALL_NUMBER) {
            float3 F0 = lerp(float3(0.04, 0.04, 0.04), payload.specular, metallic);
            float NoL = max(dot(payload.normal, L), 0.f);

            float3 H = normalize(V + L);
            float HoV = max(dot(H, V), 0.f);
            float3 F = Fresnel_Schlick(HoV, F0, roughness);

            float diffuse_lobe_selected_weight = LobeSelectionProb(payload.albedo, payload.specular);

            float NoV = max(dot(payload.normal, V), 0.f);
            float HoL = max(dot(H, L), 0.f);

            float diffuse_pdf = max(NoL / PI, SHADOWY_SMALL_NUMBER);
            float3 diffuse_weight = Diffuse_Burley_Disney(payload.albedo, F0, roughness, NoV, NoL, HoL) * NoL / diffuse_pdf;

            float specular_pdf = BRDF_PDF(V, L, payload.normal, roughness, F0);
            float3 specular_weight = CookTorranceBRDF(V, L, payload.normal, roughness, metallic, payload.albedo, F0) * NoL / specular_pdf;

            float3 Kd = (1 - F) * (1 - metallic);
            AddLobeWithMIS(mat_eval.weight, mat_eval.pdf, Kd * diffuse_weight, diffuse_pdf, diffuse_lobe_selected_weight);
            AddLobeWithMIS(mat_eval.weight, mat_eval.pdf, F * max(dot(payload.normal, L), 0.f), 1.f, 1.f - diffuse_lobe_selected_weight);
        }
    } else {
        float3 F0 = lerp(float3(0.04, 0.04, 0.04), payload.specular, metallic);
        float NoL = max(dot(payload.normal, L), 0.f);

        float3 H = normalize(V + L);
        float HoV = max(dot(H, V), 0.f);
        float3 F = Fresnel_Schlick(HoV, F0, roughness);

        float diffuse_lobe_selected_weight = LobeSelectionProb(payload.albedo, payload.specular);

        float NoV = max(dot(payload.normal, V), 0.f);
        float HoL = max(dot(H, L), 0.f);

        float diffuse_pdf = max(NoL / PI, SHADOWY_SMALL_NUMBER);
        float3 diffuse_weight = Diffuse_Burley_Disney(payload.albedo, F0, roughness, NoV, NoL, HoL) * NoL / diffuse_pdf;

        float specular_pdf = BRDF_PDF(V, L, payload.normal, roughness, F0);
        float3 specular_weight = CookTorranceBRDF(V, L, payload.normal, roughness, metallic, payload.albedo, F0) * NoL / specular_pdf;

        mat_eval.pdf = 0.f;
        mat_eval.weight = 0.f;
        float3 Kd = (1 - F) * (1 - metallic);
        AddLobeWithMIS(mat_eval.weight, mat_eval.pdf, Kd * diffuse_weight, diffuse_pdf, diffuse_lobe_selected_weight);
        AddLobeWithMIS(mat_eval.weight, mat_eval.pdf, specular_weight, specular_pdf, 1.f - diffuse_lobe_selected_weight);
    }
#endif

    return mat_eval;
}

float TargetDistribution(in ReSTIRGISample sample, in float3 V) {
#if DIFFUSE_ONLY
//     float NoL = dot(sample.second_dir, sample.vis_point_normal);
    return dot(sample.sample_point_Lo, 0.333333333.xxx);
#else
   RayPayload restir_sample_payload;
   restir_sample_payload.roughness = sample.vis_point_roughness;
   restir_sample_payload.metallic = sample.vis_point_metallic;
   restir_sample_payload.albedo = sample.vis_point_albedo;
   restir_sample_payload.normal = sample.vis_point_normal;
   MaterialEval sample_point_mat = EvalMaterial(V, sample.second_dir, restir_sample_payload);

   return dot(sample.sample_point_Lo * sample_point_mat.weight * sample_point_mat.pdf, 0.333333333.xxx);
//         return dot(sample.sample_point_Lo, 0.333333333.xxx) * sample.cos_theta;
#endif
}
