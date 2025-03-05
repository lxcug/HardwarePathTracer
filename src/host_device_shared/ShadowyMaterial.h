//
// Created by HUSTLX on 2025/1/15.
//

#ifndef SHADOWY_MATERIAL_H
#define SHADOWY_MATERIAL_H

#include "BaseDefinitions.h"
#include "Light.h"


BEGIN_SHADOWY_NAMESPACE

#define ALPHA_MODE_OPAQUE 0
#define ALPHA_MODE_TRANSPARENT 1

    /*
     * Offers Addresses for query Vertex/Index/Material Buffer
     */
    struct InstanceData
    {
        uint64_t VertexBufferAddress;
        uint64_t IndexBufferAddress;
        uint64_t MaterialBufferAddress;
        uint MaterialIndex;
    };

    /*
     * Should align with 4B cause this structure is stored in StructuredBuffer.
     * Stores material read from raw 3d asset(No texture sampling executed).
     */
    struct InputMaterial {
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float3, albedo, float3(1.f, 1.f, 1.f));
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, albedo_tex_id, -1);

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float3, emissive, float3(0.f, 0.f, 0.f));
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, emissive_tex_id, -1);

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, roughness, 1.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, normal_tex_id, -1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, metallic, 0.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, metallic_tex_id, -1);

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float3, transmission, float3(1.f, 1.f, 1.f));
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, specular_tex_id, -1);

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, opacity, 1.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, is_dds_normal_tex, 0);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, ior, 1.5f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, two_sided, 0);

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(uint, alpha_mode, ALPHA_MODE_OPAQUE);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, alpha_cutoff, 0.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float3, specular, float3(1.f, 1.f, 1.f));
    };

    /*
     * No need to align with 4B, since this structured is resolved in the closest hit or ray gen shader.
     * Stores material after texture sampling.
     */
    struct ShaderMaterial {
        float3 albedo;
        float3 transmission;
        float3 normal;
        float3 emissive;
        float3 specular;

        float roughness;
        float metallic;
        float opacity;
        float ior;

        uint alpha_mode;
        int two_sided;
        float alpha_cutoff;
    };

    /*
     * Used for AnyHit Shader Ignore hit, reduce texture query when resolving
     */
    struct AnyHitMaterial {
        float3 albedo;
        float opacity;
        int two_sided;
        uint alpha_mode;
        float alpha_cutoff;
    };

#if IS_COMPILING_SHADER
    StructuredBuffer<InstanceData> ModelInfo : register(t0, space1);
    Texture2D<float4> MaterialTextures[] : register(t1, space1);
    SamplerState Samplers[] : register(s1, space1);
    StructuredBuffer<Light> Lights : register(t2, space1);
    Texture2D<float4> SkyTexture : register(t3, space1);
    SamplerState SkyTextureSampler : register(s3, space1);

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
     * Resolve InputMaterial to ShaderMaterial
     */
    ShaderMaterial ResolveShaderMaterial(in InputMaterial input_mat, in float2 hit_uv, in float3 normal, in float4 tangent) {
        ShaderMaterial shader_mat;

        shader_mat.albedo = input_mat.albedo;
        shader_mat.opacity = input_mat.opacity;
        int texture_id = input_mat.albedo_tex_id;
        if (texture_id >= 0) {
            float4 sample_value = MaterialTextures[texture_id].SampleLevel(Samplers[texture_id], hit_uv, 0);
            shader_mat.albedo = sample_value.rgb;
            shader_mat.opacity = sample_value.a;
        }

        shader_mat.emissive = input_mat.emissive;
        texture_id = input_mat.emissive_tex_id;
        if (texture_id) {
            shader_mat.emissive = MaterialTextures[texture_id].SampleLevel(Samplers[texture_id], hit_uv, 0).rgb;
        }

        shader_mat.normal = normal;
        texture_id = input_mat.normal_tex_id;
        if (texture_id >= 0) {
            float3 local_normal = MaterialTextures[texture_id].SampleLevel(Samplers[texture_id], hit_uv, 0).rgb * 2.f - 1.f;
            float3 bitangent = cross(shader_mat.normal, tangent.xyz) * tangent.w;
            float3x3 tbn = transpose(float3x3(tangent.xyz, bitangent, shader_mat.normal));
            shader_mat.normal = normalize(mul(tbn, local_normal));
            if (input_mat.is_dds_normal_tex) {
                shader_mat.normal = -shader_mat.normal;
            }
        }

        shader_mat.roughness = input_mat.roughness;
        shader_mat.metallic = input_mat.metallic;
        texture_id = input_mat.specular_tex_id;
        if (texture_id >= 0) {
            float3 sample_value = MaterialTextures[texture_id].SampleLevel(Samplers[texture_id], hit_uv, 0).rgb;
            shader_mat.roughness = sample_value.g;
            shader_mat.metallic = sample_value.b;
        }

        shader_mat.transmission = input_mat.transmission;
        shader_mat.specular = input_mat.specular;
        shader_mat.ior = input_mat.ior;
        shader_mat.two_sided = input_mat.two_sided;
        shader_mat.alpha_mode = input_mat.alpha_mode;
        shader_mat.alpha_cutoff = input_mat.alpha_cutoff;

        return shader_mat;
    }

    /*
     * Only Resolve opacity and albedo
     */
    AnyHitMaterial ResolveAnyHitMaterial(in InputMaterial input_mat, in float2 hit_uv) {
        AnyHitMaterial any_hit_mat;

        any_hit_mat.albedo = input_mat.albedo;
        any_hit_mat.opacity = input_mat.opacity;
        int texture_id = input_mat.albedo_tex_id;
        if (texture_id >= 0) {
            float4 sample_value = MaterialTextures[texture_id].SampleLevel(Samplers[texture_id], hit_uv, 0);
            any_hit_mat.albedo = sample_value.rgb;
            any_hit_mat.opacity = sample_value.a;
        }

        any_hit_mat.two_sided = input_mat.two_sided;
        any_hit_mat.alpha_mode = input_mat.alpha_mode;
        any_hit_mat.alpha_cutoff = input_mat.alpha_cutoff;

        return any_hit_mat;
    }

#endif

END_SHADOWY_NAME_SPACE

#endif //SHADOWY_MATERIAL_H
