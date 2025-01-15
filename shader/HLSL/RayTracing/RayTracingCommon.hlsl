#include "Random.hlsl"
#include "Payload.hlsl"
#include "../../../src/host_device_shared/Vertex.h"
#include "../../../src/host_device_shared/Material.h"

struct ModelDesc {
    uint64_t VertexBufferAddress;
    uint64_t IndexBufferAddress;
    uint64_t MaterialBufferAddress;
    uint64_t MaterialIndexBufferAddress;
    int TextureIndexOffset;
};

RaytracingAccelerationStructure TLAS : register(t1, space0);
RWTexture2D<float4> OutImage : register(u2, space0);
Texture2D<float4> InImage : register(t3, space0);

// TODO: Move to space1
StructuredBuffer<ModelDesc> ModelInfo : register(t4, space0);
Texture2D<float4> MaterialTextures[] : register(t5, space0);
SamplerState Samplers[] : register(s5, space0);
RWTexture2D<float4> GBuffer[] : register(u6, space0);


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

Material GetMaterial(uint InstanceID, uint PrimitiveIndex) {
    uint64_t MaterialBufferAddress = ModelInfo[InstanceID].MaterialBufferAddress;
    uint64_t MaterialIndexBufferAddress = ModelInfo[InstanceID].MaterialIndexBufferAddress;

    int material_index = vk::RawBufferLoad<int>(MaterialIndexBufferAddress + sizeof(int) * PrimitiveIndex);
    return vk::RawBufferLoad<Material>(MaterialBufferAddress + sizeof(Material) * material_index);
}

float3 uniform_sample_hemisphere(float3 normal, float rnd1, float rnd2) {
    float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = cross(normal, tangent);
    float3 dir = float3(cos(2 * PI * rnd1) * sqrt(1 - rnd2), sin(2 * PI * rnd1) * sqrt(1 - rnd2), sqrt(rnd2));
    return dir.x * tangent + dir.y * bitangent + dir.z * normal;
}
