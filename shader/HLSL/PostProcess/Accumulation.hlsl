#pragma Compute Accumulation

#include "../../../src/host_device_shared/RenderOptions.h"
#include "../../../src/host_device_shared/ViewUniformBuffer.h"


ConstantBuffer<ViewUniformBuffer> view_uniform_buffer : register(b0, space0);
RWTexture2D<float4> SceneColor : register(u1, space0);
RWTexture2D<float4> LastFrameSceneColor : register(u2, space0);
[[vk::push_constant]] AccumulationOptions render_options;


[numthreads(16, 16, 1)]
void Accumulation(uint3 GlobalThreadID : SV_DispatchThreadID) {
    uint2 index = GlobalThreadID.xy;

    if (render_options.EnableAccumulation) {
        if (render_options.MaxAccumulatedFrames <= 0 ||
            view_uniform_buffer.AccumulatedFrameNum < render_options.MaxAccumulatedFrames) {
            float3 color = SceneColor[index].xyz;
            float3 old_color = LastFrameSceneColor[index].rgb;
            float factor = 1.f / (view_uniform_buffer.AccumulatedFrameNum + 1);
            float3 new_color = old_color * (1 - factor) + color * factor;
            SceneColor[index] = float4(new_color, 1.0);
        } else {
            SceneColor[index] = LastFrameSceneColor[index];
        }
    }

    LastFrameSceneColor[index] = SceneColor[index];
}
