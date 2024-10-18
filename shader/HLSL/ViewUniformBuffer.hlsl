cbuffer ViewUniformBuffer : register(b0) {
    float4x4 ModelTrans;
    float4x4 ViewTrans;
    float4x4 ProjTrans;
    float3 DebugColor;
    float DeltaTime;
};
