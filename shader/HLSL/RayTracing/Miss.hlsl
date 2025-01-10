#pragma shader_stage(miss)

#include "RayTracingCommon.hlsl"


[shader("miss")]
void main(inout RayPayload payload)
{
    payload.pos = float3(0., 0., 0.);
    payload.normal = float3(0., 0., 0.);
    payload.color = MISS_COLOR;
    payload.is_hit = false;
}
