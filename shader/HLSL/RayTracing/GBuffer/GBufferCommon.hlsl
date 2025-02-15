#pragma once

#include "../../../../src/host_device_shared/ViewUniformBuffer.h"
#include "../Random.hlsl"
#include "../../../../src/host_device_shared/Vertex.h"
#include "../../../../src/host_device_shared/Material.h"

struct GBufferHitAttribute
{
    float2 bary;
};

struct GBufferPayload
{
    float3 pos;
    float hit_t;

    float3 normal;
    float roughness;

    float3 albedo;
    float metallic;
};

ConstantBuffer<ViewUniformBuffer> view_uniform_buffer : register(b0, space0);
RaytracingAccelerationStructure TLAS : register(t1, space0);
RWTexture2D<float4> GBuffer[] : register(u2, space0);
RWTexture2D<float> SceneDepth : register(u3, space0);
RWTexture2D<float2> MotionVector : register(u4, space0);
RWTexture2D<float> Penumbra : register(u5, space0);
RWTexture2D<float4> Translucency : register(u6, space0);
RWTexture2D<float4> SceneColor : register(u7, space0);
