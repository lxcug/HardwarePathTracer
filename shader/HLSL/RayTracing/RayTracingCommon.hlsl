#pragma once

#include "Random.hlsl"
#include "Payload.hlsl"
#include "../../../src/host_device_shared/Vertex.h"
#include "../../../src/host_device_shared/ShadowyMaterial.h"
#include "../../../src/host_device_shared/RenderOptions.h"
#include "../../../src/host_device_shared/ReSTIRCommon.h"
#include "../../../src/host_device_shared/ViewUniformBuffer.h"


ConstantBuffer<ViewUniformBuffer> view_uniform_buffer : register(b0, space0);
RaytracingAccelerationStructure TLAS : register(t1, space0);
RWTexture2D<float4> SceneColor : register(u2, space0);
RWTexture2D<float4> LastFrameSceneColor : register(u3, space0);
RWTexture2D<float4> GBuffer[] : register(u4, space0);
// RWStructuredBuffer<ReSTIRGISample> InitialSampleBuffer : register(u5, space0);
RWTexture2D<float4> DiffuseHitDis : register(u5, space0);
RWTexture2D<float4> SpecularHitDis : register(u6, space0);
[[vk::push_constant]] PathTracingOptions render_options;
