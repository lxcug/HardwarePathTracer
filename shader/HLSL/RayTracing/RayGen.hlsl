#pragma shader_stage(raygeneration)

#include "RayTracingCommon.hlsl"
#include "../../../src/host_device_shared/ViewUniformBuffer.h"
#include "../../../src/host_device_shared/RenderOptions.h"


[shader("raygeneration")]
void main()
{
    uint2 index = DispatchRaysIndex().xy;
    uint2 dim = DispatchRaysDimensions().xy;

    uint seed = view_uniform_buffer.FrameNum + index.x * dim.y + index.y * dim.x;
    float2 jitter = float2(rnd(seed), rnd(seed));
    float2 uv = (index + .5 + jitter) / dim;
    float3 origin = view_uniform_buffer.CameraPos;
    float2 screen_coord = uv * 2.f - 1.f;
    float4 target_view_space = mul(view_uniform_buffer.InvProj, float4(screen_coord, 1.f, 1.f));
    float3 dir = mul(view_uniform_buffer.InvView, float4(normalize(target_view_space.xyz), 0.f)).xyz;

    RayDesc ray;
    ray.Origin = origin.xyz;
    ray.Direction = dir.xyz;
    ray.TMin = 1e-3f;
    ray.TMax = 1e10f;

    RayPayload payload;
    uint flags = RAY_FLAG_FORCE_OPAQUE;
    TraceRay(TLAS, flags, 0xff, 0, 0, 0, ray, payload);

    // Short Range RTAO
    float ao = 0.f;
    if (payload.is_hit) {
        float3 hit_pos = payload.pos;
        float3 hit_normal = payload.normal;
        uint hit_count = 0;
        for (int i = 0; i < render_options.NumAORays; i++) {
            RayDesc ao_ray;
            RayPayload ao_payload;
            ao_ray.Origin = hit_pos;
            ao_ray.Direction = uniform_sample_hemisphere(hit_normal, rnd(seed), rnd(seed));
            ao_ray.TMin = 1e-4;
            ao_ray.TMax = render_options.AORayLength;

            TraceRay(TLAS, flags, 0xff, 0, 0, 0, ao_ray, ao_payload);
            if (ao_payload.is_hit) {
               hit_count++;
            }
        }
        ao = 1.f - (float)hit_count / render_options.NumAORays;
    }

    if (payload.is_hit) {
        GBuffer[0][index] = float4(payload.albedo, 1.f);
        GBuffer[1][index] = float4(payload.normal, 1.f);
        GBuffer[2][index] = float4(payload.pos, 1.f);

        float4 ViewPos = mul(view_uniform_buffer.ViewTrans, float4(payload.pos, 1.0f));
        float4 ClipPos = mul(view_uniform_buffer.ProjTrans, float4(ViewPos.xyz, 1.f));
        float depth = ClipPos.z / ClipPos.w;
        GBuffer[3][index] = float4(depth, depth, depth, 1.f);

        float3 color = payload.albedo;
        if (render_options.EnableAO) {
            color *= ao;
        }

        if (view_uniform_buffer.FrameNum <= 2 || render_options.ShouldReAccumulate) {
            OutImage[index] = float4(color, 1.0);
        } else {
            float3 old_color = InImage[index].rgb;
            float3 new_color = lerp(old_color, color, 1.f / view_uniform_buffer.FrameNum);
            OutImage[index] = float4(new_color, 1.0);
        }
    } else {
        OutImage[index] = float4(0.f, 0.f, 0.f, 0.f);

        GBuffer[0][index] = float4(0.f, 0.f, 0.f, 0.f);
        GBuffer[1][index] = float4(0.f, 0.f, 0.f, 0.f);
        GBuffer[2][index] = float4(0.f, 0.f, 0.f, 0.f);
        GBuffer[3][index] = float4(0.f, 0.f, 0.f, 0.f);
    }
}
