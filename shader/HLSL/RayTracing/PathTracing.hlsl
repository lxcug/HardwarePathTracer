#include "Utils.hlsl"


float3 PathTracingKernel(in RayPayload payload, inout uint seed) {

    float3 radiance = float3(0.f, 0.f, 0.f);
    float3 path_throughput = float3(1.f, 1.f, 1.f);
    for (int bounce = 0; bounce < render_options.Bounce; bounce++) {
        bool is_camera_ray = bounce == 0;
        bool is_last_bounce = bounce == render_options.Bounce - 1;

        if (!payload.is_hit) {
            // TODO: Accumulate SkyLight
            break;
        }

        // Eval Direct Lighting
        for (int light_idx = 0; light_idx < render_options.NumLights; light_idx++) {
            Light light = Lights[light_idx];
            float3 ray_direction;
            float3 light_contrib;
            TraceLight(payload.pos, payload.normal, light, light_contrib);
            radiance += path_throughput * diffuse_lambert(payload.albedo) * light_contrib;
        }

        if (!is_last_bounce) {
            // Update PathThroughput
            // path_throughput represents brdf * cos / pdf
            float3 next_path_throughput = path_throughput * payload.albedo;  // TODO: Sample Material
            // Russian Roulette reference UnrealEngine, TODO: Use EARS or MARS to further improve quality
            float continue_prob = sqrt(max(next_path_throughput) / max(path_throughput));
            if (continue_prob < 1) {
                if (rnd(seed) >= continue_prob) {
                    break;
                }
                path_throughput = next_path_throughput / continue_prob;
            } else {
                path_throughput = next_path_throughput;
            }


            RayDesc ray;
            ray.Origin = payload.pos;
            ray.Direction = UniformSampleHemisphere(rnd2(seed), payload.normal);  // implies pdf = cos / PI
            ray.TMin = render_options.RayMinBias;
            ray.TMax = render_options.MaxTraceDistance;  // TODO: TMax as PushConstant
            TraceRay(TLAS, RAY_FLAG_NONE, 0xff, 0, 0, 0, ray, payload);
        }
    }

    return radiance;
}