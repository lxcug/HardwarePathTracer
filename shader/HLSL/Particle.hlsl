#pragma Vertex VSMain
#pragma Fragment PSMain

#include "ViewUniformBuffer.hlsl"

struct VSInput {
    float3 Position : POSITION;
    float3 PlaceHolder : PLACE_HOLDER;
    float3 Color : COLOR;
};

struct VSOutput {
    float4 ClipPosition : SV_POSITION;
    float3 Color : COLOR;
    float PointSize : PSIZE;
};

VSOutput VSMain(VSInput Input) {
    VSOutput Output;
    float4 ViewPosition = mul(ViewTrans, mul(ModelTrans, float4(Input.Position, 1.f)));
    Output.ClipPosition = mul(ProjTrans, ViewPosition);
    Output.Color = Input.Color;
    Output.PointSize = 2.f;
    return Output;
}

float4 PSMain(VSOutput Input) : SV_TARGET {
    return float4(Input.Color, 1.f);
}
