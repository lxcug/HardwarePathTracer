#pragma shader_stage(anyhit)

#include "RayTracingCommon.hlsl"


[shader("anyhit")]
void main(inout RayPayload payload, in HitAttribute attrib)
{
    float3 bary_centrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    Vertex3 Vertices = GetVertices(InstanceID(), PrimitiveIndex());
    Vertex v0 = Vertices.V0, v1 = Vertices.V1, v2 = Vertices.V2;

    float2 hit_uv = v0.TexCoord * bary_centrics.x + v1.TexCoord * bary_centrics.y + v2.TexCoord * bary_centrics.z;

    InputMaterial input_mat = GetInputMaterial(InstanceID());
    // TODO: Only access opacity and albedo texture for efficiency
    AnyHitMaterial any_hit_mat = ResolveAnyHitMaterial(input_mat, hit_uv);

    if ((any_hit_mat.alpha_mode == ALPHA_MODE_TRANSPARENT && any_hit_mat.opacity < any_hit_mat.alpha_cutoff) ||
        all(any_hit_mat.albedo <= 0.f)) {
        IgnoreHit();
    }
}
