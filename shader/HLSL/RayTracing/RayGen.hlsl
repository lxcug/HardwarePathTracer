#pragma shader_stage(raygeneration)

#include "RayTracingCommon.hlsl"
#include "../ViewUniformBuffer.hlsl"


float Random(float3 seed) {
    float x = dot(seed, float3(12.9898, 78.233, 54.53));
    x = frac(sin(x) * 43758.5453);
    return x;
}

float3 uniform_sample_hemisphere(float3 normal, float rnd1, float rnd2) {
    float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(up, normal));
    float3 bitangent = cross(normal, tangent);
    float3 dir = float3(cos(2 * PI * rnd1) * sqrt(1 - rnd2), sin(2 * PI * rnd1) * sqrt(1 - rnd2), sqrt(rnd2));
    return dir.x * tangent + dir.y * bitangent + dir.z * normal;
}

#define NUM_AO_RAYS 4


[shader("raygeneration")]
void main()
{
    uint2 index = DispatchRaysIndex().xy;
    uint2 dim = DispatchRaysDimensions().xy;

    uint seed = FrameNum + index.x * dim.y + index.y * dim.x;
    float2 jitter = float2(rnd(seed), rnd(seed));
    float2 uv = (index + .5 + jitter * .5f) / dim;
    float3 origin = CameraPos;
    float2 screen_coord = uv * 2.f - 1.f;
    float4 target_view_space = mul(InvProj, float4(screen_coord, 1.f, 1.f));
    float3 dir = mul(InvView, float4(normalize(target_view_space.xyz), 0.f)).xyz;

    RayDesc ray;
    ray.Origin = origin.xyz;
    ray.Direction = dir.xyz;
    ray.TMin = 1e-3f;
    ray.TMax = 1e10f;

    RayPayload payload;
    uint flags = RAY_FLAG_FORCE_OPAQUE;
    TraceRay(TLAS, flags, 0xff, 0, 0, 0, ray, payload);

    // Short Range RTAO
    float ao = 0.f;
    if (payload.is_hit) {
        float3 hit_pos = payload.pos;
        float3 hit_normal = payload.normal;
        uint hit_count = 0;
        for (int i = 0; i < NUM_AO_RAYS; i++) {
            RayDesc ao_ray;
            RayPayload ao_payload;
            ao_ray.Origin = hit_pos;
            ao_ray.Direction = uniform_sample_hemisphere(hit_normal, rnd(seed), rnd(seed));
            ao_ray.TMin = 1e-4;
            ao_ray.TMax = 1.f;

            TraceRay(TLAS, flags, 0xff, 0, 0, 0, ao_ray, ao_payload);
            if (ao_payload.is_hit) {
               hit_count++;
            }
        }
        ao = 1.f - (float)hit_count / NUM_AO_RAYS;
    }

    if (payload.is_hit) {
        if (FrameNum == 0 || ShouldReAccumulate) {
            OutImage[index] = float4(ao * payload.albedo, 1.0);
        } else {
            float3 old_color = InImage[index].rgb;
            float3 new_color = lerp(old_color, ao * payload.albedo, 1.f / FrameNum);
            OutImage[index] = float4(new_color, 1.0);
        }

        // OutImage[index] = float4(payload.albedo, 1.f);
    } else {
        OutImage[index] = float4(0.f, 0.f, 0.f, 0.f);
    }
}
