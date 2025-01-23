#pragma shader_stage(closesthit)

#include "../../../src/host_device_shared/ViewUniformBuffer.h"
#include "RayTracingCommon.hlsl"


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
    float3 albedo = material.Albedo;
    if (material.AlbedoTextureID >= 0) {
        int TextureIndexOffset = ModelInfo[InstanceID()].TextureIndexOffset;
        int TextureIndex = material.AlbedoTextureID + TextureIndexOffset;
        albedo = MaterialTextures[TextureIndex].SampleLevel(Samplers[TextureIndex], hit_uv, 0).rgb;
    }

    payload.pos = hit_pos;
    payload.albedo = albedo;
    payload.normal = hit_normal;
    payload.emissive = material.Emissive;
    payload.opacity = material.Opacity;
    payload.roughness = material.Roughness;
    payload.metallic = material.Metallic;
    payload.is_hit = true;
    payload.hit_t = RayTCurrent();
    payload.instance_id = InstanceID();
    payload.triangle_index = PrimitiveIndex();
}
