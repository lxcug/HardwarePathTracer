#pragma shader_stage(raygeneration)

#include "RayTracingCommon.hlsl"
#include "../../../src/host_device_shared/RenderOptions.h"
#include "BRDF.hlsl"
#include "TraceUtils.hlsl"
#include "PathTracing.hlsl"


[shader("raygeneration")]
void main()
{
    uint2 index = DispatchRaysIndex().xy;
    uint2 dim = DispatchRaysDimensions().xy;

    uint seed = tea(index.x * dim.y + index.y, view_uniform_buffer.FrameNum);

    float2 jitter = float2(rnd(seed), rnd(seed));
    float2 uv = (index + .5) / dim;
    float3 origin = view_uniform_buffer.CameraPos;
    float2 screen_coord = uv * 2.f - 1.f;
    float4 target_view_space = mul(view_uniform_buffer.InvProj, float4(screen_coord, 1.f, 1.f));
    float3 dir = mul(view_uniform_buffer.InvView, float4(normalize(target_view_space.xyz), 0.f)).xyz;

    PathTracingPayload payload;
    PathTracingKernel(origin, dir, seed, payload);

    if (payload.is_hit() || payload.hit_sky) {
        DiffuseHitDis[index] = float4(payload.diffuse_radiance, payload.hit_t);
        SpecularHitDis[index] = float4(payload.diffuse_radiance, payload.hit_t);
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
