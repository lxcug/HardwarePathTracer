#pragma once

#include "Utils.hlsl"
#include "MaterialSampling.hlsl"


struct PathTracingPayload {
    float3 radiance;
    float hit_t;

    float3 albedo;
    float ao;

    float3 normal;

    bool is_hit() {
        return hit_t >= 0.f;
    }

    bool is_miss() {
        return hit_t < 0.f;
    }
};


void PathTracingKernel(in float3 origin, in float3 direction, inout uint seed, out PathTracingPayload pt_payload) {
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = render_options.RayMinBias;
    ray.TMax = render_options.MaxTraceDistance;
    pt_payload.hit_t = -1.f;

    bool include_emissive = render_options.EnableEmissive;
    float3 radiance = float3(0.f, 0.f, 0.f);
    float3 path_throughput = float3(1.f, 1.f, 1.f);
    float3 first_pos, first_normal;
    RayPayload payload;
    for (int i = 0; i < render_options.Bounce; i++) {
        bool is_camera_ray = i == 0;
        bool is_last_bounce = (i == render_options.Bounce);

        TraceRay(TLAS, RAY_FLAG_NONE, 0xff, 0, 0, 0, ray, payload);

        if (payload.is_miss()) {
            // TODO
            if (render_options.EnableSkyLight || is_camera_ray) {
                radiance += path_throughput * sample_sky_texture(-ray.Direction);
            }
            break;
        }

        if (is_camera_ray) {
            pt_payload.hit_t = payload.hit_t;
            first_pos = payload.pos;
            first_normal = payload.normal;
        }

        if (include_emissive) {
            radiance += path_throughput * payload.emissive * payload.opacity;
        }

        // Sample Material
        MaterialSample material_sample = SampleMaterial(payload, rnd4(seed));
        if (material_sample.pdf < 1e-3f || asuint(material_sample.pdf) > 0x7f800000) {
            break;
        }

        // Update PathThroughput and Russian Roulette
        float3 next_path_throughput = path_throughput * material_sample.weight;  // TODO: Sample Material
        // Russian Roulette reference UnrealEngine, TODO: Use EARS or MARS to further improve quality
        float continue_prob = sqrt(max(next_path_throughput) / max(path_throughput));
        if (continue_prob < 1.f) {
            if (rnd(seed) >= continue_prob) {
                break;
            }
            path_throughput = next_path_throughput / continue_prob;
        } else {
            path_throughput = next_path_throughput;
        }

        // Update Ray
        ray.Origin = payload.pos;
        ray.Direction = material_sample.direction;

        // Only Sample Material Now, TODO: MIS
        for (uint idx = 0; idx < render_options.NumLights; idx++) {
            Light light = Lights[idx];

            LightHitSample hit_sample = TraceLight(ray, light);
            float3 light_contrib = hit_sample.radiance;
            if (any(light_contrib) > 0.f) {
                RayDesc light_ray = ray;
                light_ray.Direction = hit_sample.direction;
                light_ray.TMax = hit_sample.hit_t;

                light_contrib *= TraceVisibilityRay(light_ray);

                /*
                 * Only use sample material now,
                 * iterate all lights, and path_throughput / PI = brdf
                 * the following code = \sum brdf * radiance * cos
                 * TODO: sample light + material and MIS
                 */
                radiance += path_throughput / PI * light_contrib * dot(payload.normal, hit_sample.direction);
            }
        }
    }

    // Separate AO
    float ao = 1.f;
    if (render_options.EnableAO && pt_payload.is_hit()) {
        RayDesc ao_ray;
        ao_ray.Origin = first_pos;
        ao_ray.TMin = render_options.RayMinBias;
        ao_ray.TMax = render_options.AORayLength;
        ao = 0.f;
        for (int ray_idx = 0; ray_idx < render_options.NumAORays; ray_idx++) {
            ao_ray.Direction = UniformSampleHemisphere(rnd2(seed), first_normal);
            ao += TraceVisibilityRay(ao_ray);
        }
        ao /= render_options.NumAORays;
    }

    pt_payload.radiance = radiance * ao;
    pt_payload.ao = ao;
}




















