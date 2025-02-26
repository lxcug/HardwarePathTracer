//
// Created by HUSTLX on 2025/1/15.
//

#ifndef MATERIAL_H
#define MATERIAL_H

#include "BaseDefinitions.h"
#include "Light.h"


BEGIN_SHADOWY_NAMESPACE
    struct ModelDesc
    {
        // NOLINT
        uint64_t VertexBufferAddress;
        uint64_t IndexBufferAddress;
        uint64_t MaterialBufferAddress;
        uint64_t MaterialIndexBufferAddress;
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, TextureIndexOffset, -1);
    };

    struct Material
    {
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float3, Albedo, float3(.5f, .5f, .5f));
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, AlbedoTextureID, -1);

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float3, Emissive, float3(0.f, 0.f, 0.f));
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EmissiveTextureID, -1);

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float3, Transmittance, float3(1.f, 1.f, 1.f));
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, Opacity, 1.f);

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, Roughness, 1.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, Metallic, 0.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, RoughnessTextureID, -1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, MetallicTextureID, -1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, NormalTextureID, -1);
    };

#if IS_COMPILING_SHADER

// TODO: Fixed Binding to space1, Should be Consistent with the PipelineLayoutBinding Index
StructuredBuffer<ModelDesc> ModelInfo : register(t0, space1);
Texture2D<float4> MaterialTextures[] : register(t1, space1);
SamplerState Samplers[] : register(s1, space1);
StructuredBuffer<Light> Lights : register(t2, space1);
Texture2D<float4> SkyTexture : register(t3, space1);
SamplerState SkyTextureSampler : register(s3, space1);

Material GetMaterial(uint InstanceID, uint PrimitiveIndex) {
    uint64_t MaterialBufferAddress = ModelInfo[InstanceID].MaterialBufferAddress;
    uint64_t MaterialIndexBufferAddress = ModelInfo[InstanceID].MaterialIndexBufferAddress;

    int material_index = vk::RawBufferLoad<int>(MaterialIndexBufferAddress + sizeof(int) * PrimitiveIndex);
    return vk::RawBufferLoad<Material>(MaterialBufferAddress + sizeof(Material) * material_index);
}

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

#endif

END_SHADOWY_NAME_SPACE

#endif //MATERIAL_H
