#include "GlobalDefs.hlsl"
#include "../../../src/host_device_shared/RenderOptions.h"
#include "bsdf/Disney_bsdf.hlsl"
#include "bsdf/PBR_bsdf.hlsl"


struct ShadowyMaterialEval {
    float3 bsdf;
    float pdf;
};

struct ShadowyMaterialSample {
    float3 direction;
    float3 bsdf;
    float pdf;
};

ShadowyMaterialEval ShadowyEvalMaterial(
    in PathState state,
    in float3 V,
    in float3 L,
    in float3 N
) {
    ShadowyMaterialEval mat_eval;

    if (render_options.MaterialMode == MATERIAL_MODE_DISNEY) {
        ShadowyEvalDisneyMaterial(state, V, L, N, mat_eval.bsdf, mat_eval.pdf);
    } else if (render_options.MaterialMode == MATERIAL_MODE_PBR) {
        ShadowyEvalPBRMaterial(state, V, L, N, mat_eval.bsdf, mat_eval.pdf);
    }

    mat_eval.bsdf = max(mat_eval.bsdf, SHADOWY_SMALL_NUMBER);
    mat_eval.pdf = max(mat_eval.pdf, SHADOWY_SMALL_NUMBER);

    return mat_eval;
}

ShadowyMaterialSample ShadowySampleMaterial(
    in PathState state,
    in float3 V,
    in float3 N,
    inout uint seed
) {
    ShadowyMaterialSample mat_sample;

    if (render_options.MaterialMode == MATERIAL_MODE_DISNEY) {
        ShadowySampleDisneyMaterial(state, V, N, seed, mat_sample.direction, mat_sample.bsdf, mat_sample.pdf);
    } else if (render_options.MaterialMode == MATERIAL_MODE_PBR) {
        ShadowySamplePBRMaterial(state, V, N, seed, mat_sample.direction, mat_sample.bsdf, mat_sample.pdf);
    }

    mat_sample.bsdf = max(mat_sample.bsdf, SHADOWY_SMALL_NUMBER);
    mat_sample.pdf = max(mat_sample.pdf, SHADOWY_SMALL_NUMBER);

    return mat_sample;
}



