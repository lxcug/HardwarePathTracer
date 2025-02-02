#pragma shader_stage(raygeneration)

#include "RayTracingCommon.hlsl"
#include "../../../src/host_device_shared/ViewUniformBuffer.h"
#include "../../../src/host_device_shared/RenderOptions.h"
#include "BRDF.hlsl"
#include "TraceUtils.hlsl"
#include "PathTracing.hlsl"


[shader("raygeneration")]
void main()
{
    uint2 index = DispatchRaysIndex().xy;
    uint2 dim = DispatchRaysDimensions().xy;

    if (!render_options.ShouldRenderThisFrame) {
        OutImage[index] = InImage[index];
        return;
    }

    uint seed = tea(index.x * dim.y + index.y * dim.x, view_uniform_buffer.FrameNum);

    float2 jitter = float2(rnd(seed), rnd(seed));
    float2 uv = (index + .5 + jitter) / dim;
    float3 origin = view_uniform_buffer.CameraPos;
    float2 screen_coord = uv * 2.f - 1.f;
    float4 target_view_space = mul(view_uniform_buffer.InvProj, float4(screen_coord, 1.f, 1.f));
    float3 dir = mul(view_uniform_buffer.InvView, float4(normalize(target_view_space.xyz), 0.f)).xyz;

    PathTracingPayload payload;
    PathTracingKernel(origin, dir, seed, payload);

    float3 color = payload.radiance;

    if (payload.is_hit()) {
        // TODO: Accumulate for the first several frames cause artifacts, to fix
        if (view_uniform_buffer.AccumulatedFrameNum == 0) {
            OutImage[index] = float4(color, 1.0);
        } else if (render_options.ShouldRenderThisFrame) {
            float3 old_radiance = InImage[index].rgb;
            float3 new_radiance = lerp(old_radiance, color, 1.f / view_uniform_buffer.AccumulatedFrameNum);
            OutImage[index] = float4(new_radiance, 1.0);
        }
    } else {
        OutImage[index] = float4(color, 1.f);

        GBuffer[0][index] = float4(0.f, 0.f, 0.f, 0.f);
        GBuffer[1][index] = float4(0.f, 0.f, 0.f, 0.f);
        GBuffer[2][index] = float4(0.f, 0.f, 0.f, 0.f);
        GBuffer[3][index] = float4(0.f, 0.f, 0.f, 0.f);
    }
}
