#pragma Compute SpatialReuse

#include "Random.hlsl"
#include "../../../src/host_device_shared/ReSTIRCommon.h"
#include "../../../src/host_device_shared/ViewUniformBuffer.h"
#include "Payload.hlsl"
#include "BRDF.hlsl"
#include "../../../src/host_device_shared/RenderOptions.h"
#include "MaterialSampling.hlsl"


ConstantBuffer<ViewUniformBuffer> view_uniform_buffer : register(b0, space0);
StructuredBuffer<ReSTIRGIReservoir> TemporalReservoirBuffer : register(t1, space0);
RWStructuredBuffer<ReSTIRGIReservoir> SpatialReservoirBuffer : register(u2, space0);
RWTexture2D<float4> SceneColor : register(u3, space0);
RaytracingAccelerationStructure TLAS : register(t4, space0);
[[vk::push_constant]] SpatialReuseOptions render_options;


float CalJacobian(ReSTIRGISample sample, ReSTIRGISample neighbor) {
    if (neighbor.second_bounce_hit_sky) {
        return 1.f;
    }
    float3 x1r = sample.vis_point;
    float3 x1q = neighbor.vis_point;
    float3 x2q = neighbor.sample_point;
    float3 normal = neighbor.sample_point_normal;
    float cos_phi2r = dot(normalize(x1r - x2q), normal);
    float cos_phi2q = dot(normalize(x1q - x2q), normal);

    float nom = cos_phi2r * dot(x1q - x2q, x1q - x2q);
    float denom = cos_phi2q * dot(x1r - x2q, x1r - x2q);

    return min(max(nom / denom, .1f), 10.f);
}


[numthreads(16, 16, 1)]
void SpatialReuse(uint3 GlobalThreadID : SV_DispatchThreadID) {
    uint2 index = GlobalThreadID.xy;
    uint2 resolution = uint2(view_uniform_buffer.Width, view_uniform_buffer.Height);
    uint buffer_index = index.x * resolution.y + index.y;
    uint seed = tea(index.x * resolution.y + index.y, view_uniform_buffer.FrameNum);

    ReSTIRGIReservoir reservoir = TemporalReservoirBuffer[buffer_index];

    int reuse_radius = render_options.ReuseRadius;
    if (render_options.EnableSpatialReuse && reuse_radius > 0) {
        for (int xx = -reuse_radius; xx <= reuse_radius; xx++) {
            int neighbor_index_x = index.x + xx;
            if (neighbor_index_x < 0) {
                continue;
            }
            if (neighbor_index_x >= resolution.x) {
                break;
            }
            for (int yy = -reuse_radius; yy <= reuse_radius; yy++) {
                int neighbor_index_y = index.y + yy;
                if (neighbor_index_y < 0 || (xx == 0 && yy == 0)) {
                    continue;
                }
                if (neighbor_index_y >= resolution.y) {
                    break;
                }
                uint2 neighbor_index = uint2(neighbor_index_x, neighbor_index_y);
                uint neighbor_index_1D = neighbor_index.x * resolution.y + neighbor_index.y;
                ReSTIRGIReservoir neighbor_reservoir = TemporalReservoirBuffer[neighbor_index_1D];
                ReSTIRGISample neighbor_sample = neighbor_reservoir.y;
                ReSTIRGISample current_sample = reservoir.y;
                if (!neighbor_sample.is_valid_sample) {
                    continue;
                }

                float3 reused_path = neighbor_sample.sample_point - current_sample.vis_point;
                float3 neighbor_path = neighbor_sample.sample_point - neighbor_sample.vis_point;

                float dot1 = dot(current_sample.vis_point_normal, normalize(reused_path));
                float dot2 = dot(current_sample.vis_point_normal, normalize(neighbor_path));
                float normal_dot = dot(current_sample.vis_point_normal, neighbor_sample.vis_point_normal);

                bool should_reject = neighbor_reservoir.M <= 0 || dot1 <= 0.f || dot2 <= 0.f || normal_dot < .8f;
                if (should_reject) {
                    continue;
                }

                RayQuery<RAY_FLAG_CULL_NON_OPAQUE |
                         RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
                         RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;
                RayDesc ray;
                ray.Origin = current_sample.vis_point;
                float3 to_neighbor_sample_point = neighbor_sample.second_bounce_hit_sky ? neighbor_sample.second_dir : neighbor_sample.sample_point - current_sample.vis_point;
                ray.Direction = normalize(to_neighbor_sample_point);
                ray.TMin = 1e-3f;
                ray.TMax = neighbor_sample.second_bounce_hit_sky ? 1e10f : length(to_neighbor_sample_point);
                q.TraceRayInline(TLAS, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, ray);
                q.Proceed();

                float jacobian = CalJacobian(current_sample, neighbor_sample);
                neighbor_reservoir.UpdateW();
                float weight = TargetDistribution(neighbor_sample, -neighbor_sample.primary_dir) / jacobian * neighbor_reservoir.W * neighbor_reservoir.M;
                if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT) {
                    weight = 0.f;
                }

                int M = reservoir.M;
                reservoir.Update(neighbor_sample, weight, seed);
                if (TargetDistribution(neighbor_sample, -neighbor_sample.primary_dir) > 0.f) {
                    reservoir.M = M + neighbor_reservoir.M;
                } else {
                    reservoir.M = M;
                }
            }
        }

        reservoir.UpdateW();
        RayPayload restir_sample_payload;
        restir_sample_payload.roughness = reservoir.y.vis_point_roughness;
        restir_sample_payload.metallic = reservoir.y.vis_point_metallic;
        restir_sample_payload.base_color = reservoir.y.vis_point_albedo;
        restir_sample_payload.normal = reservoir.y.vis_point_normal;
        MaterialEval sample_point_mat = EvalMaterial(-reservoir.y.primary_dir, reservoir.y.second_dir, restir_sample_payload);

        float3 color = reservoir.RISEstimator() * sample_point_mat.weight * sample_point_mat.pdf;
        SceneColor[index] = float4(color, 1.0);
        SpatialReservoirBuffer[buffer_index] = reservoir;
    }
}
