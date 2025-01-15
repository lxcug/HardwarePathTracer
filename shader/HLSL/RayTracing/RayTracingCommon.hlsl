#include "Random.hlsl"
#include "Payload.hlsl"
#include "../../../src/host_device_shared/Vertex.h"
#include "../../../src/host_device_shared/Material.h"


RaytracingAccelerationStructure TLAS : register(t1, space0);
RWTexture2D<float4> OutImage : register(u2, space0);
Texture2D<float4> InImage : register(t3, space0);
RWTexture2D<float4> GBuffer[] : register(u4, space0);


float3 uniform_sample_hemisphere(float3 normal, float rnd1, float rnd2) {
    float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = cross(normal, tangent);
    float3 dir = float3(cos(2 * PI * rnd1) * sqrt(1 - rnd2), sin(2 * PI * rnd1) * sqrt(1 - rnd2), sqrt(rnd2));
    return dir.x * tangent + dir.y * bitangent + dir.z * normal;
}
