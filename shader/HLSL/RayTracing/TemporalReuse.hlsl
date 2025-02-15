#pragma Compute TemporalReuse

#include "Random.hlsl"
#include "../../../src/host_device_shared/ReSTIRCommon.h"
#include "../../../src/host_device_shared/ViewUniformBuffer.h"
#include "Payload.hlsl"
#include "../../../src/host_device_shared/RenderOptions.h"
#include "MaterialSampling.hlsl"


ConstantBuffer<ViewUniformBuffer> view_uniform_buffer : register(b0, space0);
StructuredBuffer<ReSTIRGISample> InitialSampleBuffer : register(t1);
RWStructuredBuffer<ReSTIRGIReservoir> TemporalReservoirBuffer : register(u2);
StructuredBuffer<ReSTIRGIReservoir> LastFrameTemporalReservoirBuffer : register(t3);
RWTexture2D<float4> SceneColor : register(u4);
[[vk::push_constant]] TemporalReuseOptions render_options;


[numthreads(16, 16, 1)]
void TemporalReuse(uint3 GlobalThreadID : SV_DispatchThreadID) {
    uint2 index = GlobalThreadID.xy;
    uint2 resolution = uint2(view_uniform_buffer.Width, view_uniform_buffer.Height);
    uint buffer_index = index.x * resolution.y + index.y;
    uint seed = tea(index.x * resolution.y + index.y, view_uniform_buffer.FrameNum);

    ReSTIRGISample sample = InitialSampleBuffer[buffer_index];

    ReSTIRGIReservoir temporal_reservoir;
    if (view_uniform_buffer.AccumulatedFrameNum == 0) {
        temporal_reservoir = (ReSTIRGIReservoir)0;
    } else {
        temporal_reservoir = LastFrameTemporalReservoirBuffer[buffer_index];
    }

    if (render_options.MaxAccumulatedFrames <= 0 || view_uniform_buffer.AccumulatedFrameNum < render_options.MaxAccumulatedFrames) {
        float sample_weight = TargetDistribution(sample, -sample.primary_dir) / SourceDistribution(sample);
        temporal_reservoir.Update(sample, sample_weight, seed);
        temporal_reservoir.UpdateW();
    }

    RayPayload restir_sample_payload;
    restir_sample_payload.roughness = temporal_reservoir.y.vis_point_roughness;
    restir_sample_payload.metallic = temporal_reservoir.y.vis_point_metallic;
    restir_sample_payload.albedo = temporal_reservoir.y.vis_point_albedo;
    restir_sample_payload.normal = temporal_reservoir.y.vis_point_normal;
    MaterialEval sample_point_mat = EvalMaterial(-temporal_reservoir.y.primary_dir, temporal_reservoir.y.second_dir, restir_sample_payload);

    float3 color = temporal_reservoir.RISEstimator() * sample_point_mat.weight * sample_point_mat.pdf;

    SceneColor[index] = float4(color, 1.f);
    TemporalReservoirBuffer[buffer_index] = temporal_reservoir;
}
