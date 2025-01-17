#pragma once

#include "Random.hlsl"
#include "Payload.hlsl"
#include "../../../src/host_device_shared/Vertex.h"
#include "../../../src/host_device_shared/Material.h"


RaytracingAccelerationStructure TLAS : register(t1, space0);
RWTexture2D<float4> OutImage : register(u2, space0);
Texture2D<float4> InImage : register(t3, space0);
RWTexture2D<float4> GBuffer[] : register(u4, space0);
