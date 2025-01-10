#pragma shader_stage(closesthit)

#include "RayTracingCommon.hlsl"


[shader("closesthit")]
void main(inout RayPayload payload, in HitAttribute attrib)
{
    float3 hit_pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 hit_normal = mul(attrib.normal, (float3x3)ObjectToWorld3x4());

    payload.pos = hit_pos;
    payload.normal = hit_normal;
    payload.color = HIT_COLOR;
    payload.is_hit = true;
}
