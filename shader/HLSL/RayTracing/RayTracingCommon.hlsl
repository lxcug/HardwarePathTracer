#include "Random.hlsl"
#include "Payload.hlsl"
#include "../../../src/host_device_shared/Vertex.h"
#include "../../../src/host_device_shared/Material.h"


RaytracingAccelerationStructure TLAS : register(t1, space0);
RWTexture2D<float4> OutImage : register(u2, space0);
Texture2D<float4> InImage : register(t3, space0);
RWTexture2D<float4> GBuffer[] : register(u4, space0);


float3 uniform_sample_hemisphere(float rnd1, float rnd2) {
    float3 dir = float3(cos(2 * PI * rnd1) * sqrt(1 - rnd2), sin(2 * PI * rnd1) * sqrt(1 - rnd2), sqrt(rnd2));
    return dir;
}

void GetCoordBasis(in float3 normal, out float3 tangent, out float3 bitangent) {
    float3 z  = normal;
    const float yz = -z.y * z.z;
    bitangent = normalize(((abs(z.z) > 0.99999f) ? float3(-z.x * z.y, 1.0f - z.y * z.y, yz) : float3(-z.x * z.z, yz, 1.0f - z.z * z.z)));

    tangent = cross(bitangent, z);
}
