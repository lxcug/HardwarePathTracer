#pragma shader_stage(miss)

#include "GBufferCommon.hlsl"


[shader("miss")]
void main(inout GBufferPayload payload)
{
    payload.hit_t = -1.f;
}
