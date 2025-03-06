#pragma shader_stage(miss)

#include "RayTracingCommon.hlsl"


[shader("miss")]
void main(inout RayPayload payload)
{
    payload.hit_t = -1.f;
}
