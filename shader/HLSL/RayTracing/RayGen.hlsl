#pragma shader_stage(raygeneration)

#include "RayTracingCommon.hlsl"
#include "../../../src/host_device_shared/RenderOptions.h"
#include "BRDF.hlsl"
#include "TraceUtils.hlsl"
#include "ShadowyPathTracing.hlsl"


[shader("raygeneration")]
void main()
{
    uint2 index = DispatchRaysIndex().xy;
    uint2 dim = DispatchRaysDimensions().xy;

    uint seed = tea(index.x * dim.y + index.y, view_uniform_buffer.FrameNum);

    float2 jitter = rnd2(seed) / 2.f;
	// TODO: Remove jitter when using denoiser
    float2 uv = (index + .5 + jitter) / dim;
    float3 origin = view_uniform_buffer.CameraPos;
    float2 screen_coord = uv * 2.f - 1.f;
    float4 target_view_space = mul(view_uniform_buffer.InvProj, float4(screen_coord, 1.f, 1.f));
    float3 dir = mul(view_uniform_buffer.InvView, float4(normalize(target_view_space.xyz), 0.f)).xyz;

    PathTracingReturn pt_ret;
    PathTracing(origin, dir, seed, pt_ret);

    if (render_options.RayDirection.w > 0.f && all(index == 0)) {
        RayDesc picking_ray;
        picking_ray.Origin = view_uniform_buffer.CameraPos;
        picking_ray.Direction = render_options.RayDirection.xyz;
        picking_ray.TMin = render_options.RayMinBias;
        picking_ray.TMax = render_options.MaxTraceDistance;
        RayPayload picking_payload;
        TraceRay(TLAS, RAY_FLAG_NONE, 0xff, 0, 0, 0, picking_ray, picking_payload);
        if (picking_payload.is_hit()) {
            PickingInstanceID[0] = picking_payload.instance_id;
        } else {
            PickingInstanceID[0] = -2;
        }
    }

    InstanceIDTex[index] = -1;
    if (pt_ret.is_hit() || pt_ret.hit_sky) {
        DiffuseHitDis[index] = float4(pt_ret.radiance, pt_ret.hit_t);
        SpecularHitDis[index] = float4(pt_ret.radiance, pt_ret.hit_t);
        if (pt_ret.is_hit()) {
            InstanceIDTex[index] = pt_ret.instance_id;
        }
    } else {
        DiffuseHitDis[index] = float4(0.f, 0.f, 0.f, 0.f);
        SpecularHitDis[index] = float4(0.f, 0.f, 0.f, 0.f);
    }

    SceneColor[index] = DiffuseHitDis[index];

//     bool enable_restir = render_options.EnableReSTIRGI;
//     if (enable_restir) {
//         uint buffer_index = index.x * dim.y + index.y;
//
//         if (render_options.MaxAccumulatedFrames <= 0 || view_uniform_buffer.AccumulatedFrameNum < render_options.MaxAccumulatedFrames) {
//             bool is_valid_sample = false;
//             ReSTIRGISample restir_sample = (ReSTIRGISample)0;
//             RayPayload ray_payload;
//
//             PathTracingKernelReSTIR(origin, dir, seed, restir_sample, ray_payload, is_valid_sample);
//             InitialSampleBuffer[buffer_index] = restir_sample;
//         }
//     } else {
//         if (render_options.MaxAccumulatedFrames > 0 &&
//                 view_uniform_buffer.AccumulatedFrameNum >= render_options.MaxAccumulatedFrames) {
//             SceneColor[index] = LastFrameSceneColor[index];
//             return;
//         }
//
//         PathTracingPayload payload;
//         PathTracingKernel(origin, dir, seed, payload);
//
//         float3 color = payload.radiance;
//
//         if (payload.is_hit() || payload.hit_sky) {
//             if (view_uniform_buffer.AccumulatedFrameNum == 0) {
//                 SceneColor[index] = float4(color, 1.0);
//             }  else if (render_options.MaxAccumulatedFrames <= 0 || view_uniform_buffer.AccumulatedFrameNum < render_options.MaxAccumulatedFrames)  {
//                 float3 old_radiance = LastFrameSceneColor[index].rgb;
//                 float3 new_radiance = lerp(old_radiance, color, 1.f / view_uniform_buffer.AccumulatedFrameNum);
//                 SceneColor[index] = float4(new_radiance, 1.0);
//             }
//         } else {
//             SceneColor[index] = float4(0.f, 0.f, 0.f, 1.f);
//         }
//
//         LastFrameSceneColor[index] = SceneColor[index];
//     }
}
