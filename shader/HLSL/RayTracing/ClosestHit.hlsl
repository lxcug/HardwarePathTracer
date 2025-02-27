#pragma shader_stage(closesthit)

#include "../../../src/host_device_shared/ViewUniformBuffer.h"
#include "RayTracingCommon.hlsl"
#include "Utils.hlsl"


[shader("closesthit")]
void main(inout RayPayload payload, in HitAttribute attrib)
{
    float3 bary_centrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    Vertex3 Vertices = GetVertices(InstanceID(), PrimitiveIndex());
    Vertex v0 = Vertices.V0, v1 = Vertices.V1, v2 = Vertices.V2;

    float2 hit_uv = v0.TexCoord * bary_centrics.x + v1.TexCoord * bary_centrics.y + v2.TexCoord * bary_centrics.z;
    float3 hit_pos_obj_space = v0.Pos * bary_centrics.x + v1.Pos * bary_centrics.y + v2.Pos * bary_centrics.z;
    float3 hit_normal_obj_space = v0.Normal * bary_centrics.x + v1.Normal * bary_centrics.y + v2.Normal * bary_centrics.z;
    float3 hit_normal = normalize(mul(WorldToObject4x3(), hit_normal_obj_space).xyz);
    float3 hit_pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    // hit_pos = mul(float4(hit_pos_obj_space, 1.f), ObjectToWorld4x3());

    uint64_t MaterialBufferAddress = ModelInfo[InstanceID()].MaterialBufferAddress;
    uint64_t MaterialIndexBufferAddress = ModelInfo[InstanceID()].MaterialIndexBufferAddress;

    int material_index = vk::RawBufferLoad<int>(MaterialIndexBufferAddress + sizeof(int) * PrimitiveIndex());

    Material material = GetMaterial(InstanceID(), PrimitiveIndex());
    int TextureIndexOffset = ModelInfo[InstanceID()].TextureIndexOffset;
    float3 albedo = material.Albedo;
    float opacity = material.Opacity;
    if (material.AlbedoTextureID >= 0) {
        int TextureIndex = material.AlbedoTextureID + TextureIndexOffset;
        float4 Value = MaterialTextures[TextureIndex].SampleLevel(Samplers[TextureIndex], hit_uv, 0);
        albedo = Value.rgb;
        opacity = Value.a;
    }
    float roughness = material.Roughness;
    if (material.RoughnessTextureID >= 0) {
        int TextureIndex = material.RoughnessTextureID + TextureIndexOffset;
        roughness = MaterialTextures[TextureIndex].SampleLevel(Samplers[TextureIndex], hit_uv, 0).r;
    }
    float metallic = material.Metallic;
    if (material.MetallicTextureID >= 0) {
        int TextureIndex = material.MetallicTextureID + TextureIndexOffset;
        metallic = MaterialTextures[TextureIndex].SampleLevel(Samplers[TextureIndex], hit_uv, 0).r;
    }
    float3 emissive = material.Emissive;
    if (material.EmissiveTextureID >= 0) {
        int TextureIndex = material.EmissiveTextureID + TextureIndexOffset;
        emissive = MaterialTextures[TextureIndex].SampleLevel(Samplers[TextureIndex], hit_uv, 0).rgb;
    }
    if (material.NormalTextureID >= 0) {
        int TextureIndex = material.NormalTextureID + TextureIndexOffset;
        float3 local_normal = MaterialTextures[TextureIndex].SampleLevel(Samplers[TextureIndex], hit_uv, 0).rgb * 2.f - 1.f;
        float3 tangent = v0.Tangent.xyz;
        float3 bitangent = cross(hit_normal, tangent) * v0.Tangent.w;
        float3x3 TBN = transpose(float3x3(tangent, bitangent, hit_normal));
        hit_normal = normalize(mul(TBN, local_normal));
        if (material.IsDDSNormalTexture) {  // NOTE: Flip normal when using dds format texture
            hit_normal *= -1;
        }
    }
    if (material.SpecularTextureID >= 0) {
        int TextureIndex = material.SpecularTextureID + TextureIndexOffset;
        float4 Value = MaterialTextures[TextureIndex].SampleLevel(Samplers[TextureIndex], hit_uv, 0);
        float occlusion = Value.r;
        roughness = Value.g;
        metallic = Value.b;
    }

    payload.pos = hit_pos;
    payload.albedo = albedo;
    payload.normal = hit_normal;
    payload.emissive = emissive;
    payload.opacity = opacity;
    payload.roughness = roughness;
    payload.metallic = metallic;
    payload.is_hit = true;
    payload.hit_t = RayTCurrent();
    payload.instance_id = InstanceID();
    payload.triangle_index = PrimitiveIndex();
}
