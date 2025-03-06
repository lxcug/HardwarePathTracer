

// TODO
void ShadowyEvalPBRMaterial(
    in PathState state,
    in float3 V,
    in float3 L,
    in float3 N,
    inout float3 out_brdf,
    inout float out_pdf
) {

}

// TODO
void ShadowySamplePBRMaterial(
    in PathState state,
    in float3 V,
    in float3 N,
    inout uint seed,
    inout float3 out_dir,
    inout float3 out_brdf,
    inout float out_pdf
) {
    float3 L;
    out_dir = L;
    ShadowyEvalDisneyMaterial(state, V, L, N, out_brdf, out_pdf);
}
