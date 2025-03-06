#include "GlobalDefs.hlsl"
#include "../../../src/host_device_shared/RenderOptions.h"
#include "BXDF/Disney_BXDF.hlsl"
#include "BXDF/PBR_BXDF.hlsl"


struct ShadowyMaterialEval {
    float3 bxdf;
    float pdf;
};

struct ShadowyMaterialSample {
    float3 direction;
    float3 bxdf;
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
        ShadowyEvalDisneyMaterial(state, V, L, N, mat_eval.bxdf, mat_eval.pdf);
    } else if (render_options.MaterialMode == MATERIAL_MODE_PBR) {
        ShadowyEvalPBRMaterial(state, V, L, N, mat_eval.bxdf, mat_eval.pdf);
    }

    mat_eval.bxdf = max(mat_eval.bxdf, SHADOWY_SMALL_NUMBER);
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
        ShadowySampleDisneyMaterial(state, V, N, seed, mat_sample.direction, mat_sample.bxdf, mat_sample.pdf);
    } else if (render_options.MaterialMode == MATERIAL_MODE_PBR) {
        ShadowySamplePBRMaterial(state, V, N, seed, mat_sample.direction, mat_sample.bxdf, mat_sample.pdf);
    }

    mat_sample.bxdf = max(mat_sample.bxdf, SHADOWY_SMALL_NUMBER);
    mat_sample.pdf = max(mat_sample.pdf, SHADOWY_SMALL_NUMBER);

    return mat_sample;
}



