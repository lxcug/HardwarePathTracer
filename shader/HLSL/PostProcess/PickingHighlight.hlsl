#pragma Compute PickingHighlight

RWTexture2D<int> InstanceIDTex : register(u0);
RWStructuredBuffer<int> PickingInstanceID : register(u1);
RWTexture2D<float4> SceneColor : register(u2);


[numthreads(16, 16, 1)]
void PickingHighlight(uint3 GlobalThreadID : SV_DispatchThreadID) {
    uint2 index = GlobalThreadID.xy;

    const float3 highlight_color = float3(0.5, 1.2, 1.5);
    if (PickingInstanceID[0] == InstanceIDTex[index].x) {
        SceneColor[index].rgb = highlight_color;
    }
}
