cbuffer ViewUniformBuffer : register(b0) {
    float4x4 ModelTrans;
    float4x4 ViewTrans;
    float4x4 ProjTrans;

    float3 DebugColor;
    float DeltaTime;

    float3 CameraPos;
    uint FrameNum;

    float4x4 InvView;
    float4x4 InvProj;
};
