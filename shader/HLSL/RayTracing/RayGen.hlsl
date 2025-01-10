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

#define AO_RAYS 4


[shader("raygeneration")]
void main()
{
    uint2 index = DispatchRaysIndex().xy;
    uint2 dim = DispatchRaysDimensions().xy;
    float aspect_ratio = float(dim.x) / float(dim.y);

    float2 uv = (index + .5) / dim;
    // TODO: Bind View Uniform Buffer
    float3 origin = CameraPos;
    float2 screen_coord = uv * 2.0f - 1.0f;

    float3 dir = normalize(float3(screen_coord.x * aspect_ratio, screen_coord.y, -2.f));

    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = dir;
    ray.TMin = 1e-3f;
    ray.TMax = 1e10f;

    RayPayload payload;
    uint flags = RAY_FLAG_FORCE_OPAQUE;
    TraceRay(TLAS, flags, 0xff, 0, 0, 0, ray, payload);

    // Short Range RTAO
    float ao = 1.f;
    if (payload.is_hit) {
        float rnds[AO_RAYS * 2];
        for (int i = 0; i < 10; i++) {
            rnds[i] = Random(payload.pos + float3(i, 0.0, 0.0) + float3(DispatchRaysIndex().xy, 1));
        }

        uint hit_count = 0;
        for (int i = 0; i < AO_RAYS; i++) {
            RayDesc ray;
            ray.Origin = payload.pos ;
            ray.Direction = uniform_sample_hemisphere(payload.normal, rnds[2 * i], rnds[2 * i + 1]);
            ray.TMin = 1e-4;
            ray.TMax = 1.f;

            RayPayload payload;
            uint flags = RAY_FLAG_FORCE_OPAQUE;
            TraceRay(TLAS, flags, 0xff, 0, 0, 0, ray, payload);
            if (payload.is_hit) {
               hit_count++;
            }
        }
        ao = 1.f - hit_count / AO_RAYS;
    }


    OutImage[index] = float4(ao * payload.color, 1.0);
}
