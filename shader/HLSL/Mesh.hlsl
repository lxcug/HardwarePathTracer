#pragma Vertex VSMain
#pragma Fragment PSMain

#include "ViewUniformBuffer.hlsl"

struct VSInput {
    float3 Position : POSITION;
    float3 Color : COLOR;
    float2 TexCoord : TEXCOORD0;
};

struct VSOutput {
    float4 ClipPosition : SV_POSITION;
    float3 Color : COLOR;
    float2 TexCoord : TEXCOORD0;
};

Texture2D TexSampler : register(t1);
SamplerState SamplerState : register(s1);

VSOutput VSMain(VSInput Input) {
    VSOutput Output;
    float4 ViewPosition = mul(ViewTrans, mul(ModelTrans, float4(Input.Position, 1.f)));
    Output.ClipPosition = mul(ProjTrans, ViewPosition);
    Output.Color = Input.Color;
    Output.TexCoord = Input.TexCoord;
    return Output;
}

float4 PSMain(VSOutput Input) : SV_TARGET {
    float3 TexColor = TexSampler.Sample(SamplerState, Input.TexCoord).xyz;
    return float4(TexColor, 1.f);
}
