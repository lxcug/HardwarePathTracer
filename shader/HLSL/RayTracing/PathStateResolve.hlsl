#pragma once

#include "GlobalDefs.hlsl"
#include "../../../src/host_device_shared/ShadowyMaterial.h"

/*
 * Get Instance Material via InstanceID
 */
InputMaterial GetInputMaterial(uint InstanceID) {
    uint64_t MaterialBufferAddress = ModelInfo[InstanceID].MaterialBufferAddress;
    return vk::RawBufferLoad<InputMaterial>(MaterialBufferAddress + sizeof(InputMaterial) * ModelInfo[InstanceID].MaterialIndex);
}

/*
 * Get Triangle Vertices via InstanceID and PrimitiveID
 */
Vertex3 GetVertices(uint InstanceID, uint PrimitiveIndex) {
    uint64_t VertexBufferAddress = ModelInfo[InstanceID].VertexBufferAddress;
    uint64_t IndexBufferAddress = ModelInfo[InstanceID].IndexBufferAddress;
    uint3 Indices = {
        vk::RawBufferLoad<uint>(IndexBufferAddress + sizeof(uint) * (PrimitiveIndex * 3 + 0)),
        vk::RawBufferLoad<uint>(IndexBufferAddress + sizeof(uint) * (PrimitiveIndex * 3 + 1)),
        vk::RawBufferLoad<uint>(IndexBufferAddress + sizeof(uint) * (PrimitiveIndex * 3 + 2))
    };
    Vertex3 Ret = {
        vk::RawBufferLoad<Vertex>(VertexBufferAddress + sizeof(Vertex) * Indices.x),
        vk::RawBufferLoad<Vertex>(VertexBufferAddress + sizeof(Vertex) * Indices.y),
        vk::RawBufferLoad<Vertex>(VertexBufferAddress + sizeof(Vertex) * Indices.z)
    };

    return Ret;
}

/*
 * Only Resolve opacity and base_color
 */
AnyHitMaterial ResolveAnyHitMaterial(in InputMaterial input_mat, in float2 hit_uv) {
    AnyHitMaterial any_hit_mat;

    any_hit_mat.base_color = input_mat.base_color;
    any_hit_mat.opacity = input_mat.opacity;
    int texture_id = input_mat.base_color_tex_id;
    if (texture_id >= 0) {
        float4 sample_value = MaterialTextures[texture_id].SampleLevel(Samplers[texture_id], hit_uv, 0);
        any_hit_mat.base_color = sample_value.rgb;
        any_hit_mat.opacity *= sample_value.a;
    }

    any_hit_mat.two_sided = input_mat.two_sided;
    any_hit_mat.alpha_mode = input_mat.alpha_mode;
    any_hit_mat.alpha_cutoff = input_mat.alpha_cutoff;

    return any_hit_mat;
}

void ResolvePathStateGeometry(
    inout PathState state,
    in RayDesc ray,
    in RayPayload payload
) {
    uint64_t material_buffer_addr = ModelInfo[payload.instance_id].MaterialBufferAddress;
    state.mat_addr = material_buffer_addr + sizeof(InputMaterial) * ModelInfo[payload.instance_id].MaterialIndex;

    state.pos = ray.Origin + ray.Direction * payload.hit_t;

    float3 bary_centrics = float3(1.f - payload.bary.x - payload.bary.y, payload.bary.x, payload.bary.y);

    Vertex3 vertices = GetVertices(payload.instance_id, payload.primitive_id);
    Vertex v0 = vertices.V0, v1 = vertices.V1, v2 = vertices.V2;

    // Process normal
    float3 local_normal = v0.Normal * bary_centrics.x + v1.Normal * bary_centrics.y + v2.Normal * bary_centrics.z;
    float3 world_normal = normalize(mul((float3x3)payload.world2obj, local_normal));
    float3 geo_normal = normalize(cross(v1.Pos - v0.Pos, v2.Pos - v0.Pos));
    float3 world_geo_normal = normalize(mul((float3x3)payload.world2obj, geo_normal));
    // Some issues with world normal
    state.normal = world_normal;

    // Process tangent and bitangent
    float3 local_tangent = normalize(v0.Tangent.xyz * bary_centrics.x + v1.Tangent.xyz * bary_centrics.y + v2.Tangent.xyz * bary_centrics.z);
    float3 world_tangent = normalize(mul(float4(local_tangent, 0.f), payload.obj2world)).xyz;
    float3 world_bitangent = cross(world_normal, world_tangent) * v0.Tangent.w;
    state.tangent = world_tangent;
    state.bitangent = world_bitangent;

    state.uv = v0.TexCoord * bary_centrics.x + v1.TexCoord * bary_centrics.y + v2.TexCoord * bary_centrics.z;

    if (dot(world_geo_normal, state.normal) <= 0.f) {
        state.normal = -state.normal;
    }
}


void ResolvePathStateMaterial(
    inout PathState state,
    in RayDesc ray
) {
    InputMaterial input_mat = vk::RawBufferLoad<InputMaterial>(state.mat_addr);

    state.mat.base_color = input_mat.base_color;
    state.mat.opacity = input_mat.opacity;
    int texture_id = input_mat.base_color_tex_id;
    if (texture_id >= 0) {
        float4 sample_value = MaterialTextures[texture_id].SampleLevel(Samplers[texture_id], state.uv, 0);
        state.mat.base_color *= sample_value.rgb;
        state.mat.opacity *= sample_value.a;
    }
    
    texture_id = input_mat.normal_tex_id;
    state.face_front_normal = dot(state.normal, -ray.Direction) > 0.f ? state.normal : -state.normal;
    if (texture_id >= 0) {
        float3 local_normal = normalize(MaterialTextures[texture_id].SampleLevel(Samplers[texture_id], state.uv, 0).rgb * 2.f - 1.f);
        float3 tangent = state.tangent;
        float3 bitangent = state.bitangent;
        float3x3 tbn = float3x3(tangent, bitangent, state.normal);
        float3 world_normal = normalize(mul(local_normal, tbn));
        state.normal = world_normal;
        state.face_front_normal = dot(state.normal, -ray.Direction) > 0.f ? state.normal : -state.normal;
    }

    state.mat.roughness = input_mat.roughness;
    state.mat.metallic = input_mat.metallic;
    texture_id = input_mat.specular_tex_id;
    if (texture_id >= 0) {
        float3 sample_value = MaterialTextures[texture_id].SampleLevel(Samplers[texture_id], state.uv, 0).rgb;
        state.mat.roughness = sample_value.g;
        state.mat.metallic = sample_value.b;
    }
    state.mat.roughness = max(state.mat.roughness, 1e-3f);  // NOTE: clamp min roughness to 1e-3f

    // TODO: transmission texture_id
    state.mat.transmission = input_mat.transmission;

    state.mat.emission = input_mat.emission;
    state.mat.specular = 0.f;
    state.mat.specular_tint = input_mat.specular_tint;
    state.mat.ior = input_mat.ior;
    state.mat.two_sided = input_mat.two_sided;
    state.mat.alpha_mode = input_mat.alpha_mode;
    state.mat.alpha_cutoff = input_mat.alpha_cutoff;

    state.eta = dot(state.normal, state.face_front_normal) > 0.f ? 1.f / state.mat.ior : state.mat.ior;

    state.mat.subsurface = input_mat.subsurface;

    // KHR_materials_clearcoat
    state.mat.clearcoat = input_mat.clearcoat;
    state.mat.clearcoat_roughness = max(input_mat.clearcoat_roughness, 1e-3f);

    // KHR_materials_sheen
    state.mat.sheen = input_mat.sheen;
    state.mat.sheen_tint = input_mat.sheen_tint;

    state.mat.anisotropy = input_mat.anisotropy;
    float aspect = sqrt(1.f - input_mat.anisotropy * .9f);
    state.mat.ax = max(1e-3f, state.mat.roughness / aspect);
    state.mat.ay = max(1e-3f, state.mat.roughness * aspect);
}
