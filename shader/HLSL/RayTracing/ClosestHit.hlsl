#pragma shader_stage(closesthit)

#include "RayTracingCommon.hlsl"


void VertexToWorld(inout Vertex v, float4x3 obj2world, float4x3 world2obj) {
    v.Pos = mul(float4(v.Pos, 1.f), obj2world);

    v.Normal = normalize(mul(world2obj, v.Normal).xyz);
}


[shader("closesthit")]
void main(inout RayPayload payload, in HitAttribute attrib)
{
    uint instance_id = InstanceID();
    uint triangle_index = PrimitiveIndex();
    uint vert_id = 3 * triangle_index;
    float3 bary_centrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    uint64_t VertexBufferAddress = ModelInfo[instance_id].VertexBufferAddress;
    uint64_t IndexBufferAddress = ModelInfo[instance_id].IndexBufferAddress;
    uint64_t MaterialBufferAddress = ModelInfo[instance_id].MaterialBufferAddress;
    uint64_t MaterialIndexBufferAddress = ModelInfo[instance_id].MaterialIndexBufferAddress;
    int TextureIndexOffset = ModelInfo[instance_id].TextureIndexOffset;

    uint3 indices = {
        vk::RawBufferLoad<uint>(IndexBufferAddress + sizeof(uint) * (vert_id + 0)),
        vk::RawBufferLoad<uint>(IndexBufferAddress + sizeof(uint) * (vert_id + 1)),
        vk::RawBufferLoad<uint>(IndexBufferAddress + sizeof(uint) * (vert_id + 2))
    };
    Vertex v0 = vk::RawBufferLoad<Vertex>(VertexBufferAddress + sizeof(Vertex) * indices.x);
    Vertex v1 = vk::RawBufferLoad<Vertex>(VertexBufferAddress + sizeof(Vertex) * indices.y);
    Vertex v2 = vk::RawBufferLoad<Vertex>(VertexBufferAddress + sizeof(Vertex) * indices.z);
    int triangle_material_index = vk::RawBufferLoad<int>(MaterialIndexBufferAddress + sizeof(int) * triangle_index);
    Material triangle_material = vk::RawBufferLoad<Material>(MaterialBufferAddress + sizeof(Material) * triangle_material_index);

    float3 albedo = triangle_material.Albedo;
    int TextureID = triangle_material.TextureID;

    if (TextureID >= 0) {
        float2 hit_uv = v0.TexCoord * bary_centrics.x + v1.TexCoord * bary_centrics.y + v2.TexCoord * bary_centrics.z;
        int TextureIndex = TextureID + TextureIndexOffset;
        albedo = MaterialTextures[TextureIndex].SampleLevel(Samplers[TextureIndex], hit_uv, 0).rgb;
    }

    float3 hit_pos_obj_space = v0.Pos * bary_centrics.x + v1.Pos * bary_centrics.y + v2.Pos * bary_centrics.z;
    float3 hit_normal_obj_space = v0.Normal * bary_centrics.x + v1.Normal * bary_centrics.y + v2.Normal * bary_centrics.z;
    float3 hit_normal = normalize(mul(WorldToObject4x3(), hit_normal_obj_space).xyz);

    float3 hit_pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    hit_pos = mul(float4(hit_pos_obj_space, 1.f), ObjectToWorld4x3());

    float3 color = v0.Color * bary_centrics.x + v1.Color * bary_centrics.y + v2.Color * bary_centrics.z;

    float TexID = triangle_material.TextureID;
    payload.pos = hit_pos;
    payload.albedo = albedo;
    payload.normal = hit_normal;
    payload.is_hit = true;
    payload.hit_t = RayTCurrent();
    payload.instance_id = instance_id;
    payload.triangle_index = triangle_index;
}
