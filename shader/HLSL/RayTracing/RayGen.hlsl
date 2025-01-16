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

    uint seed = tea(index.x * dim.y + index.y * dim.x, view_uniform_buffer.FrameNum);

    float2 jitter = float2(rnd(seed), rnd(seed));
    float2 uv = (index + .5 + jitter) / dim;
    float3 origin = view_uniform_buffer.CameraPos;
    float2 screen_coord = uv * 2.f - 1.f;
    float4 target_view_space = mul(view_uniform_buffer.InvProj, float4(screen_coord, 1.f, 1.f));
    float3 dir = mul(view_uniform_buffer.InvView, float4(normalize(target_view_space.xyz), 0.f)).xyz;

    RayDesc ray;
    ray.Origin = origin.xyz;
    ray.Direction = dir.xyz;
    ray.TMin = render_options.RayMinBias;
    ray.TMax = 1e3f;

    RayPayload payload;
    uint flags = RAY_FLAG_FORCE_OPAQUE;
    TraceRay(TLAS, flags, 0xff, 0, 0, 0, ray, payload);

    float3 radiance = float3(0.f, 0.f, 0.f);
    if (payload.is_hit) {
        radiance = PathTracingKernel(payload, seed);
    }

    // Distance Based RT AO
    float ao = 0.f;
    if (render_options.EnableAO) {
        if (payload.is_hit) {
            uint hit_count = 0;
            float3 hit_pos = payload.pos;
            float3 tangent, bitangent;
            GetCoordBasis(payload.normal, tangent, bitangent);
            for (int i = 0; i < render_options.NumAORays; i++) {
                float3 dir_local = UniformSampleHemisphere(rnd(seed), rnd(seed));
                float3 dir_world = dir_local.x * tangent + dir_local.y * bitangent + dir_local.z * payload.normal;

                ao += TraceAORay(hit_pos, dir_world, rnd2(seed));
            }
            ao /= (render_options.NumAORays * render_options.AORayLength);
        }
    } else {
        ao = 1.f;
    }

    if (payload.is_hit) {
        GBuffer[0][index] = float4(payload.albedo, 1.f);
        GBuffer[1][index] = float4(payload.normal, 1.f);
        GBuffer[2][index] = float4(payload.pos, 1.f);

        float4 ViewPos = mul(view_uniform_buffer.ViewTrans, float4(payload.pos, 1.0f));
        float4 ClipPos = mul(view_uniform_buffer.ProjTrans, float4(ViewPos.xyz, 1.f));
        float depth = ClipPos.z / ClipPos.w;
        GBuffer[3][index] = float4(depth, depth, depth, 1.f);

        float3 color = radiance * ao;

        // TODO: Accumulate for the first several frames cause artifacts, to fix
        if (view_uniform_buffer.FrameNum <= 5 || render_options.ShouldReAccumulate || !render_options.Accumulation) {
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
