#pragma once

#include "BRDF.hlsl"
#include "PBRT_BRDF.hlsl"


#define DIFFUSE_ONLY 0
#define ENABLE_TRANSMISSION 0


struct MaterialSample {
    float3 direction;
    float pdf;

    float3 weight;  // brdf * cos / pdf
    float roughness;
};

MaterialSample SampleDiffuseLambert(in float3 V, in RayPayload payload, in float4 rnd) {
    MaterialSample mat_sample;

    float4 sample = CosineSampleHemisphere(rnd.xy, payload.normal);
    mat_sample.direction = sample.xyz;
    mat_sample.pdf = sample.w;
    mat_sample.weight = payload.albedo;

    return mat_sample;
}

MaterialSample SampleSpecular(in float3 V, in RayPayload payload, in float4 rnd) {
    MaterialSample mat_sample;

    float3 L = reflect(-V, payload.normal);
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), payload.albedo, payload.metallic);
    float NoL = max(dot(payload.normal, L), 0.f);
    float3 F = Fresnel_Schlick(dot(V, payload.normal), F0, payload.roughness);

    mat_sample.direction = L;
    mat_sample.pdf = 1.f;
    mat_sample.weight = F * NoL;

    return mat_sample;
}

MaterialSample SampleSpecularTransmission(in float3 V, in RayPayload payload, in float4 rnd) {
    MaterialSample mat_sample;
    float roughness = payload.roughness, metallic = payload.metallic;
	mat_sample.weight = 0.f;
    mat_sample.pdf = 0.f;

    float eta = payload.is_front_face ? 1.f / payload.ior : payload.ior / 1.f;
    float3 normal = payload.is_front_face ? payload.normal : -payload.normal;
    float3 reflect_dir = reflect(-V, normal);
    float CosTheta = dot(V, normal);
    float R = FrDielectric(CosTheta, eta) * payload.opacity;  // Reflection Prob
    float T = 1 - R;  // Transmission Prob

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), payload.albedo, metallic);
    float3 F = Fresnel_Schlick(CosTheta, F0, roughness);
    float3 transmittance = (1.f).xxx;
    float3 refract_dir = refract(-V, normal, eta);
    float refract_NoL = dot(normal, refract_dir);

	float3 L;
    if (rnd.z < T) {
        L = refract_dir;
        float refract_pdf = 1.f;
        float3 refract_weight = transmittance / refract_pdf;
    	AddLobeWithMIS(mat_sample.weight, mat_sample.pdf, refract_weight, refract_pdf, 1.f);
    } else {
        L = reflect_dir;
        float reflect_NoL = max(dot(reflect_dir, payload.normal), 0.f);
        float reflect_pdf = 1.f;
        float3 reflect_weight = F * reflect_NoL / reflect_pdf;
        AddLobeWithMIS(mat_sample.weight, mat_sample.pdf, reflect_weight, reflect_pdf, 1.f);
    }
	mat_sample.direction = L;

    return mat_sample;
}

MaterialSample SampleSpecularDiffuse(in float3 V, in RayPayload payload, in float4 rnd) {
    MaterialSample mat_sample;
    mat_sample.pdf = 0.f;
    float roughness = payload.roughness, metallic = payload.metallic;

    float3 L = reflect(-V, payload.normal);
    mat_sample.direction = L;

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), payload.albedo, metallic);
    float NoL = max(dot(payload.normal, L), 0.f);

    float HoV = max(dot(payload.normal, V), 0.f);
    float3 F = Fresnel_Schlick(HoV, F0, roughness);

    float diffuse_lobe_selected_weight = LobeSelectionProb(payload.albedo, payload.albedo);

    float NoV = max(dot(payload.normal, V), 0.f);
    float3 H = normalize(V + L);
    float HoL = max(dot(H, L), 0.f);

    float diffuse_pdf = max(NoL / PI, SHADOWY_SMALL_NUMBER);
    float3 diffuse_weight = Diffuse_Burley_Disney(payload.albedo, F0, roughness, NoV, NoL, HoL) * NoL / diffuse_pdf;

    float specular_pdf = 1.f;
    float3 specular_weight = F * NoL;

    float3 Kd = (1 - F) * (1 - metallic);
    AddLobeWithMIS(mat_sample.weight, mat_sample.pdf, Kd * diffuse_weight, diffuse_pdf, diffuse_lobe_selected_weight);
    AddLobeWithMIS(mat_sample.weight, mat_sample.pdf, specular_weight, specular_pdf, (1.f - diffuse_lobe_selected_weight));

    return mat_sample;
}

MaterialSample SampleGlossyDiffuse(in float3 V, in RayPayload payload, in float4 rnd) {
    MaterialSample mat_sample;
	mat_sample.pdf = 0.f;
    float roughness = payload.roughness, metallic = payload.metallic;

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), payload.albedo, metallic);
    float3 F = Fresnel_Schlick(max(dot(payload.normal, V), 0.0), F0, roughness);

    float diffuse_lobe_selected_weight = LobeSelectionProb(payload.albedo, payload.albedo);

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

    float3 Kd = (1 - F) * (1 - metallic);
    AddLobeWithMIS(mat_sample.weight, mat_sample.pdf, Kd * diffuse_weight, diffuse_pdf, diffuse_lobe_selected_weight);
    AddLobeWithMIS(mat_sample.weight, mat_sample.pdf, specular_weight, specular_pdf, 1.f - diffuse_lobe_selected_weight);

    return mat_sample;
}

MaterialSample SampleGlossyDiffuseTransmission(in float3 V, in RayPayload payload, in float4 rnd) {
    MaterialSample mat_sample;
    mat_sample.weight = 0.f;
    mat_sample.pdf = 0.f;
    float roughness = payload.roughness, metallic = payload.metallic;

    float eta = payload.is_front_face ? 1.f / payload.ior : payload.ior / 1.f;  // TODO: Get IOR

    float3 transmittance = (1.f).xxx;
    float4 sample = SampleGGXNormal(rnd.xy, V, payload.normal, roughness);
    float3 sample_normal = payload.is_front_face ? sample.xyz : -sample.xyz;
    float3 refract_dir = refract(-V, sample_normal, eta);
    float3 refract_H = sample_normal;

    float R = FrDielectric(dot(V, refract_H), eta) * payload.opacity;  // Reflection Prob
    float T = (1 - R);  // Transmission Prob

    float3 oriented_normal = payload.normal;
    if (dot(payload.normal, refract_H) < 0.f)
        oriented_normal = -payload.normal;

    float diffuse_lobe_selected_weight = LobeSelectionProb(payload.albedo, payload.albedo);
    float4 diffuse_lobe_dir = CosineSampleHemisphere(rnd.xy, payload.normal);
    float4 glossy_lobe_dir = SampleGGXReflection(rnd.xy, V, payload.normal, roughness);

    float3 L;
    if (rnd.z < T) {  // Transmission Only
        L = refract_dir;
        float refract_pdf = BTDF_PDF(V, L, oriented_normal, refract_H, roughness, eta);
        float3 refract_weight = transmittance * BTDF(V, L, oriented_normal, refract_H, roughness, eta) / refract_pdf;
        AddLobeWithMIS(mat_sample.weight, mat_sample.pdf, refract_weight, refract_pdf, 1.f);
    } else {  // Diffuse + Glossy Lobe
        if (rnd.w < diffuse_lobe_selected_weight) {
            L = diffuse_lobe_dir.xyz;
        } else {
            L = glossy_lobe_dir.xyz;
        }
        float NoL = max(dot(payload.normal, L), 0.f);
        float3 H = normalize(V + L);
        float HoV = max(dot(H, V), 0.f);
        float NoV = max(dot(payload.normal, V), 0.f);
        float HoL = max(dot(H, L), 0.f);
        float3 F0 = lerp(float3(0.04, 0.04, 0.04), payload.albedo, metallic);
        float3 F = Fresnel_Schlick(max(dot(payload.normal, V), 0.0), F0, roughness);

        float diffuse_pdf = max(NoL / PI, SHADOWY_SMALL_NUMBER);
        float3 diffuse_weight = Diffuse_Burley_Disney(payload.albedo, F0, roughness, NoV, NoL, HoL) * NoL / diffuse_pdf;

        float specular_pdf = BRDF_PDF(V, L, payload.normal, roughness, F0);
        float3 specular_weight = CookTorranceBRDF(V, L, payload.normal, roughness, metallic, payload.albedo, F0) * NoL / specular_pdf;

        float3 Kd = (1 - F) * (1 - metallic);
        AddLobeWithMIS(mat_sample.weight, mat_sample.pdf, Kd * diffuse_weight, diffuse_pdf, diffuse_lobe_selected_weight);
        AddLobeWithMIS(mat_sample.weight, mat_sample.pdf, specular_weight, specular_pdf, (1.f - diffuse_lobe_selected_weight));
    }
    mat_sample.direction = L;

    return mat_sample;
}

MaterialSample SampleMaterial(in float3 ray_direction, in RayPayload payload, in float4 rnd) {
    static float specular_threshold = 5e-2f;
    float roughness = payload.roughness;
    float metallic = payload.metallic;
    float3 V = -ray_direction;

#if DIFFUSE_ONLY
    return SampleDiffuseLambert(V, payload, rnd);
#else
    // Dealing with Ideal Reflection/Transmission
	if (roughness < specular_threshold) {
#if ENABLE_TRANSMISSION
		if (payload.opacity < .99f) {
			return SampleSpecularTransmission(V, payload, rnd);
		}
#endif
		return SampleSpecularDiffuse(V, payload, rnd);
	} else {
#if ENABLE_TRANSMISSION
		if (payload.opacity < .99f) {
			return SampleGlossyDiffuseTransmission(V, payload, rnd);
		}
#endif
	    return SampleGlossyDiffuse(V, payload, rnd);
	}

#endif
}

struct MaterialEval
{
	float3 weight;  // brdf * cos / pdf
	float pdf;
};

MaterialEval EvalDiffuseLambert(in float3 V, in float3 L, in RayPayload payload, inout uint seed) {
    MaterialEval mat_eval;
    float NoL = max(dot(payload.normal, L), 0.f);
    mat_eval.weight = payload.albedo;
    mat_eval.pdf = max(NoL / PI, SHADOWY_SMALL_NUMBER);

    return mat_eval;
}

MaterialEval EvalSpecularDiffuse(in float3 V, in float3 L, in RayPayload payload, inout uint seed) {
    MaterialEval mat_eval;
    mat_eval.weight = 0.f;
    mat_eval.pdf = 0.f;
    float roughness = payload.roughness, metallic = payload.metallic;

    float3 H = normalize(V + L);
    if (dot(H, payload.normal) < SHADOWY_SMALL_NUMBER) {
        float3 F0 = lerp(float3(0.04, 0.04, 0.04), payload.albedo, metallic);
        float NoL = max(dot(payload.normal, L), 0.f);

        float3 H = normalize(V + L);
        float HoV = max(dot(H, V), 0.f);
        float3 F = Fresnel_Schlick(HoV, F0, roughness);

        float diffuse_lobe_selected_weight = LobeSelectionProb(payload.albedo, payload.albedo);

        float NoV = max(dot(payload.normal, V), 0.f);
        float HoL = max(dot(H, L), 0.f);

        float diffuse_pdf = max(NoL / PI, SHADOWY_SMALL_NUMBER);
        float3 diffuse_weight = Diffuse_Burley_Disney(payload.albedo, F0, roughness, NoV, NoL, HoL) * NoL / diffuse_pdf;

        float specular_pdf = BRDF_PDF(V, L, payload.normal, roughness, F0);
        float3 specular_weight = CookTorranceBRDF(V, L, payload.normal, roughness, metallic, payload.albedo, F0) * NoL / specular_pdf;

        float3 Kd = (1 - F) * (1 - metallic);
        if (dot(L, payload.normal) > 0.f) {
            AddLobeWithMIS(mat_eval.weight, mat_eval.pdf, Kd * diffuse_weight, diffuse_pdf, diffuse_lobe_selected_weight);
            AddLobeWithMIS(mat_eval.weight, mat_eval.pdf, F * max(dot(payload.normal, L), 0.f), 1.f, 1.f - diffuse_lobe_selected_weight);
        }
    }

    return mat_eval;
}

MaterialEval EvalSpecularTransmission(in float3 V, in float3 L, in RayPayload payload, inout uint seed) {
    MaterialEval mat_eval;

    return mat_eval;
}

MaterialEval EvalGlossyDiffuseTransmission(in float3 V, in float3 L, in RayPayload payload, inout uint seed) {
    MaterialEval mat_eval;
    mat_eval.weight = 0.f;
    mat_eval.pdf = 0.f;
    float roughness = payload.roughness, metallic = payload.metallic;

    float eta = payload.is_front_face ? 1.f / 2.f : 2.f / 1.f;  // TODO: Get IOR

    float3 transmittance = (1.f).xxx;
    float3 refract_dir = L;
    float3 refract_H = normalize(L * eta + V);

    float R = FrDielectric(dot(V, refract_H), eta) * payload.opacity;  // Reflection Prob
    float T = (1 - R);  // Transmission Prob

    float3 oriented_normal = payload.normal;
    if (dot(payload.normal, refract_H) < 0.f)
        oriented_normal = -payload.normal;

    float refract_pdf = BTDF_PDF(V, refract_dir, oriented_normal, refract_H, roughness, eta);
    float3 refract_weight = BTDF(V, refract_dir, oriented_normal, refract_H, roughness, eta) / refract_pdf;

    if (dot(refract_dir, refract_H) > 0.f) {
        refract_weight = 0.f;
    }
    uint prob = rnd(seed);
    if (prob < T) {
        AddLobeWithMIS(mat_eval.weight, mat_eval.pdf, refract_weight, refract_pdf, R);
    }

    float diffuse_lobe_selected_weight = LobeSelectionProb(payload.albedo, payload.albedo);

    float NoL = max(dot(payload.normal, L), 0.f);
    float3 H = normalize(V + L);
    float HoV = max(dot(H, V), 0.f);
    float NoV = max(dot(payload.normal, V), 0.f);
    float HoL = max(dot(H, L), 0.f);
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), payload.albedo, metallic);
    float3 F = Fresnel_Schlick(max(dot(payload.normal, V), 0.0), F0, roughness);

    float diffuse_pdf = max(NoL / PI, SHADOWY_SMALL_NUMBER);
    float3 diffuse_weight = Diffuse_Burley_Disney(payload.albedo, F0, roughness, NoV, NoL, HoL) * NoL / diffuse_pdf;

    float specular_pdf = BRDF_PDF(V, L, payload.normal, roughness, F0);
    float3 specular_weight = CookTorranceBRDF(V, L, payload.normal, roughness, metallic, payload.albedo, F0) * NoL / specular_pdf;

    float3 Kd = (1 - F) * (1 - metallic);
    if (prob < R || dot(L, payload.normal) > 0.f) {
        AddLobeWithMIS(mat_eval.weight, mat_eval.pdf, Kd * diffuse_weight, diffuse_pdf, diffuse_lobe_selected_weight * R);
        AddLobeWithMIS(mat_eval.weight, mat_eval.pdf, specular_weight, specular_pdf, (1.f - diffuse_lobe_selected_weight) * R);
    }

    return mat_eval;
}

MaterialEval EvalGlossyDiffuse(in float3 V, in float3 L, in RayPayload payload, inout uint seed) {
    MaterialEval mat_eval;
    mat_eval.weight = 0.f;
    mat_eval.pdf = 0.f;
    float roughness = payload.roughness, metallic = payload.metallic;

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), payload.albedo, metallic);
    float3 F = Fresnel_Schlick(max(dot(payload.normal, V), 0.0), F0, roughness);

    float NoL = max(dot(payload.normal, L), 0.f);
    float3 H = normalize(V + L);
    float HoV = max(dot(H, V), 0.f);
    float NoV = max(dot(payload.normal, V), 0.f);
    float HoL = max(dot(H, L), 0.f);

    float diffuse_lobe_selected_weight = LobeSelectionProb(payload.albedo, payload.albedo);

    float diffuse_pdf = max(NoL / PI, SHADOWY_SMALL_NUMBER);
    float3 diffuse_weight = Diffuse_Burley_Disney(payload.albedo, F0, roughness, NoV, NoL, HoL) * NoL / diffuse_pdf;

    float specular_pdf = BRDF_PDF(V, L, payload.normal, roughness, F0);
    float3 specular_weight = CookTorranceBRDF(V, L, payload.normal, roughness, metallic, payload.albedo, F0) * NoL / specular_pdf;

    float3 Kd = (1 - F) * (1 - metallic);
     if (dot(L, payload.normal) > 0.f) {
        AddLobeWithMIS(mat_eval.weight, mat_eval.pdf, Kd * diffuse_weight, diffuse_pdf, diffuse_lobe_selected_weight);
        AddLobeWithMIS(mat_eval.weight, mat_eval.pdf, specular_weight, specular_pdf, 1.f - diffuse_lobe_selected_weight);
    }

    return mat_eval;
}

MaterialEval EvalMaterial(in float3 V, in float3 L, in RayPayload payload, inout uint seed) {
    MaterialEval mat_eval;
    static float specular_threshold = 5e-2f;
    float roughness = payload.roughness;

#if DIFFUSE_ONLY
    return EvalDiffuseLambert(V, L, payload, seed);
#else
    // Dealing with Ideal Reflection/Transmission
    if (roughness < specular_threshold) {
        return EvalSpecularDiffuse(V, L, payload, seed);

        // Don't use NEE for Dielectric Material
#if ENABLE_TRANSMISSION
        if (payload.opacity < .99f) {
            return EvalSpecularTransmission(V, L, payload, seed);
        }
#endif
    } else {
        return EvalGlossyDiffuse(V, L, payload, seed);

        // Don't use NEE for Dielectric Material
#if ENABLE_TRANSMISSION
        // if (payload.opacity < .99f) {
        //     return EvalGlossyDiffuseTransmission(V, L, payload, seed);
        // }
#endif
        // return EvalGlossyDiffuse(V, L, payload, seed);
    }
#endif
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
   uint seed = 1u;
   MaterialEval sample_point_mat = EvalMaterial(V, sample.second_dir, restir_sample_payload, seed);  // TODO: seed

   return dot(sample.sample_point_Lo * sample_point_mat.weight * sample_point_mat.pdf, 0.333333333.xxx);
//         return dot(sample.sample_point_Lo, 0.333333333.xxx) * sample.cos_theta;
#endif
}
