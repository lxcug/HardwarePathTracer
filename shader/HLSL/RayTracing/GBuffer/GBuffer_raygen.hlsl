#include "GBufferCommon.hlsl"


[shader("raygeneration")]
void main()
{
    uint2 index = DispatchRaysIndex().xy;
    uint2 dim = DispatchRaysDimensions().xy;

    uint seed = tea(index.x * dim.y + index.y, view_uniform_buffer.FrameNum);

    float2 uv = (index + .5) / dim;
    float3 origin = view_uniform_buffer.CameraPos;
    float2 screen_coord = uv * 2.f - 1.f;
    float4 target_view_space = mul(view_uniform_buffer.InvProj, float4(screen_coord, 1.f, 1.f));
    float3 dir = mul(view_uniform_buffer.InvView, float4(normalize(target_view_space.xyz), 0.f)).xyz;

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = dir;
    ray.TMin = 1e-3f;
    ray.TMax = 1e10f;  // TODO

    GBufferPayload payload;
    TraceRay(TLAS, RAY_FLAG_NONE, 0xff, 0, 0, 0, ray, payload);

    if (payload.hit_t > 0.f) {
        GBuffer[0][index] = float4(payload.albedo, payload.metallic);
        GBuffer[1][index] = float4(payload.normal, payload.roughness);
        GBuffer[2][index] = float4(payload.pos, 1.f);
        SceneDepth[index] = length(payload.pos - view_uniform_buffer.CameraPos);
        MotionVector[index] = float2(0.f, 0.f);
        Penumbra[index] = 0.f;  // TODO
        Translucency[index] = float4(0.f, 0.f, 0.f, 0.f);
    } else {
        GBuffer[0][index] = float4(0.f, 0.f, 0.f, 0.f);
        GBuffer[1][index] = float4(0.f, 0.f, 0.f, 0.f);
        GBuffer[2][index] = float4(0.f, 0.f, 0.f, 0.f);
        SceneDepth[index] = 0.f;
        MotionVector[index] = float2(0.f, 0.f);
        Penumbra[index] = 0.f;
        Translucency[index] = float4(0.f, 0.f, 0.f, 0.f);
    }

    //SceneColor[index] = float4(GBuffer[0][index]);
}
