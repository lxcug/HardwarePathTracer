#pragma once

#include "Utils.hlsl"
#include "MaterialSampling.hlsl"
#include "LightSampling.hlsl"
#include "../../../src/host_device_shared/ReSTIRCommon.h"


struct PathTracingPayload {
    float3 radiance;
    float hit_t;

    float3 diffuse_radiance;
    float3 specular_radiance;

    float3 albedo;
    float ao;

    float3 normal;
    bool hit_sky;

    bool is_hit() {
        return hit_t >= 0.f;
    }

    bool is_miss() {
        return hit_t < 0.f;
    }
};

struct TraceMaterialState {
    float3 radiance;
    LightType light_type;
    float distance;
};

void PathTracingKernel(in float3 origin,
                       in float3 direction,
                       inout uint seed,
                       out PathTracingPayload pt_payload) {
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = normalize(direction);
    ray.TMin = render_options.RayMinBias;
    ray.TMax = render_options.MaxTraceDistance;
    pt_payload.hit_t = -1.f;
    pt_payload.hit_sky = false;

    bool include_emissive = render_options.EnableEmissive;
    float3 radiance = float3(0.f, 0.f, 0.f);
    float3 path_throughput = float3(1.f, 1.f, 1.f);
    float3 first_pos, first_normal;
    RayPayload payload;

    bool use_MIS = render_options.MISMode == 2;
    bool b_sample_light = render_options.MISMode == 0;
    bool b_sample_material = render_options.MISMode == 1;
    float light_pick_cdf[64];
    TraceMaterialState trace_material_state[64];
    uint valid_num_trace_light_state = 0;

    [loop]
    for (int bounce = 0; bounce < render_options.Bounce; bounce++) {
        bool is_camera_ray = bounce == 0;
        bool is_last_bounce = (bounce == render_options.Bounce - 1);
        bool should_accumulate_radiance = !render_options.ShowIndirectOnly || bounce > 0;
        bool should_accumulate_radiance_for_deferred_trace_light = !render_options.ShowIndirectOnly || bounce > 1;

        TraceRay(TLAS, RAY_FLAG_NONE, 0xff, 0, 0, 0, ray, payload);

        if (render_options.DeferredTraceMaterial && !b_sample_light) {
            for (int idx = 0; idx < valid_num_trace_light_state; idx++) {
                TraceMaterialState curr_state = trace_material_state[idx];
                if ((payload.is_miss() || payload.hit_t >= curr_state.distance) && should_accumulate_radiance_for_deferred_trace_light) {
                    radiance += curr_state.radiance;
                }
            }
            valid_num_trace_light_state = 0;
        }

        if (payload.is_miss()) {
            // For Displaying SkyTexture
            if (render_options.EnableSkyLight && is_camera_ray) {
                pt_payload.hit_sky = true;
                radiance += path_throughput * SkyTexture.SampleLevel(SkyTextureSampler, uint_vector_to_hdri_uv(-ray.Direction), 0).rgb;
            }
            break;
        }

        if (is_camera_ray) {
            pt_payload.hit_t = payload.hit_t;
            first_pos = payload.pos;
            first_normal = payload.normal;
        }

        if (include_emissive && should_accumulate_radiance) {
            radiance += path_throughput * payload.emissive * payload.opacity;
        }

        float4 random_sample = rnd4(seed);

        float light_pick_cdf_sum = 0.f;
        if ((b_sample_light || use_MIS) && render_options.NumLights > 0) {
            float3 pos = payload.pos;
            float3 normal = payload.normal;

            for (int idx = 0; idx < render_options.NumLights; idx++) {
                Light light = Lights[idx];
                light_pick_cdf_sum += EstimateLight(light, pos, normal);
                light_pick_cdf[idx] = light_pick_cdf_sum;
            }

            if (light_pick_cdf_sum > 0.f) {
                uint selected_light_index;
                float selected_light_pdf;
                SelectLight(random_sample.x * light_pick_cdf_sum, light_pick_cdf, selected_light_index, selected_light_pdf);
                selected_light_pdf /= light_pick_cdf_sum;

                Light selected_light = Lights[selected_light_index];
                LightSample light_sample = SampleLight(selected_light, random_sample.yz, pos, normal);
                light_sample.radiance_over_pdf /= selected_light_pdf;
                light_sample.pdf *= selected_light_pdf;

                if (light_sample.pdf > 0.f) {
                    RayDesc light_ray;
                    light_ray.Origin = pos;
                    light_ray.Direction = light_sample.direction;
                    light_ray.TMin = render_options.RayMinBias;
                    light_ray.TMax = light_sample.distance;

                    light_sample.radiance_over_pdf *= TraceVisibilityRay(light_ray);
                }

                if (any(light_sample.radiance_over_pdf) > 0.f && should_accumulate_radiance) {
                    MaterialEval material_eval = EvalMaterial(-ray.Direction, light_sample.direction, payload);
                    float3 light_contrib = path_throughput * light_sample.radiance_over_pdf * material_eval.weight * material_eval.pdf;
                    if (use_MIS) {
                        light_contrib *= MISWeightRobust(light_sample.pdf, material_eval.pdf);
                    }
                    radiance += light_contrib;
                }
            }
        }

        // Sample Material
        MaterialSample material_sample = SampleMaterial(ray.Direction, payload, random_sample);
        if (material_sample.pdf <= SHADOWY_SMALL_NUMBER || asuint(material_sample.pdf) > 0x7F800000) {
//             radiance = float3(0, 0, 100.f);
            break;
        }

        // Update PathThroughput and Russian Roulette
        float3 next_path_throughput = path_throughput * material_sample.weight;
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
        ray.TMin = render_options.RayMinBias;
        ray.TMax = render_options.MaxTraceDistance;

        if ((b_sample_material || use_MIS) && render_options.NumLights > 0) {
            for (uint idx = 0; idx < render_options.NumLights; idx++) {
                Light light = Lights[idx];
                LightHitSample hit_sample = TraceLight(ray, light);
                if (hit_sample.is_miss()) {
                    continue;
                }
                float3 light_contrib = path_throughput * hit_sample.radiance;

                if (use_MIS && light_pick_cdf_sum > 0.f) {
                    float prev_cdf = idx > 0 ? light_pick_cdf[idx - 1] : 0.0;
                    float light_pick_pdf = (light_pick_cdf[idx] - prev_cdf) / light_pick_cdf_sum;
                    light_contrib *= MISWeightRobust(material_sample.pdf, hit_sample.pdf * light_pick_pdf);
                }

                if (render_options.DeferredTraceMaterial) {
                    TraceMaterialState curr_state;
                    curr_state.radiance = light_contrib;
                    curr_state.light_type = light.Type;
                    curr_state.distance = hit_sample.hit_t;
                    trace_material_state[valid_num_trace_light_state++] = curr_state;
                } else {
                    RayDesc light_ray = ray;
                    light_ray.TMax = hit_sample.hit_t;
                    light_contrib *= TraceVisibilityRay(light_ray);
                    if (should_accumulate_radiance) {
                        radiance += light_contrib;
                    }
                }
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
            ao_ray.Direction = UniformSampleHemisphere(rnd2(seed), first_normal).xyz;
            ao += TraceVisibilityRay(ao_ray);
        }
        ao /= render_options.NumAORays;
    }

    pt_payload.radiance = radiance;
    pt_payload.ao = ao;
    pt_payload.diffuse_radiance = radiance;
}

void PathTracingKernelReSTIR(in float3 origin,
                             in float3 direction,
                             inout uint seed,
                             out ReSTIRGISample restir_sample,
                             out RayPayload ray_payload,
                             out bool is_valid_sample) {
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = render_options.RayMinBias;
    ray.TMax = render_options.MaxTraceDistance;
    restir_sample.primary_dir = direction;

    bool include_emissive = render_options.EnableEmissive;
    float3 radiance = float3(0.f, 0.f, 0.f);
    float3 second_bounce_radiance = float3(0.f, 0.f, 0.f);
    float3 path_throughput = float3(1.f, 1.f, 1.f);
    float3 second_bounce_thp = float3(1.f, 1.f, 1.f);
    float3 first_pos, first_normal;
    RayPayload payload;

    bool use_MIS = render_options.MISMode == 2;
    bool b_sample_light = render_options.MISMode == 0;
    bool b_sample_material = render_options.MISMode == 1;
    float light_pick_cdf[64];
    TraceMaterialState trace_material_state[64];
    uint valid_num_trace_light_state = 0;
    is_valid_sample = false;
    restir_sample.second_bounce_hit_sky = false;
    restir_sample.is_valid_sample = false;

    [loop]
    for (int bounce = 0; bounce < render_options.Bounce; bounce++) {
        bool is_camera_ray = bounce == 0;
        bool is_last_bounce = (bounce == render_options.Bounce - 1);
        bool should_accumulate_radiance = !render_options.ShowIndirectOnly || bounce > 0;
        bool should_accumulate_radiance_for_deferred_trace_light = !render_options.ShowIndirectOnly || bounce > 1;

        TraceRay(TLAS, RAY_FLAG_NONE, 0xff, 0, 0, 0, ray, payload);

        if (render_options.DeferredTraceMaterial && !b_sample_light) {
            for (int idx = 0; idx < valid_num_trace_light_state; idx++) {
                TraceMaterialState curr_state = trace_material_state[idx];
                if ((payload.is_miss() || payload.hit_t >= curr_state.distance) && should_accumulate_radiance_for_deferred_trace_light) {
                    radiance += path_throughput * curr_state.radiance;
                    if (bounce >= 2) {
                        second_bounce_radiance += curr_state.radiance * second_bounce_thp;
                    }
                }
            }
            valid_num_trace_light_state = 0;
        }

        if (is_camera_ray) {
            ray_payload = payload;
        }

        if (payload.is_miss()) {
            // For Displaying SkyTexture
            if (render_options.EnableSkyLight && is_camera_ray) {
                radiance += path_throughput * SkyTexture.SampleLevel(SkyTextureSampler, uint_vector_to_hdri_uv(-ray.Direction), 0).rgb;
            }
            if (bounce == 1) {
                restir_sample.second_bounce_hit_sky = true;
            }
            break;
        }

        if (bounce == 1) {
            restir_sample.sample_point = payload.pos;
            restir_sample.sample_point_normal = payload.normal;
            restir_sample.cos_theta = dot(restir_sample.vis_point_normal, normalize(payload.pos - restir_sample.vis_point));
            is_valid_sample = true;
        }

        if (is_camera_ray) {
            restir_sample.vis_point = payload.pos;
            restir_sample.vis_point_normal = payload.normal;
            restir_sample.vis_point_roughness = payload.roughness;
            restir_sample.vis_point_metallic = payload.metallic;
            restir_sample.vis_point_albedo = payload.albedo;
            restir_sample.is_valid_sample = true;
        }

        if (include_emissive && should_accumulate_radiance) {
            radiance += path_throughput * payload.emissive * payload.opacity;
            if (bounce >= 1) {
                second_bounce_radiance += second_bounce_thp * payload.emissive * payload.opacity;
            }
        }

        float4 random_sample = rnd4(seed);

        float light_pick_cdf_sum = 0.f;
        if ((b_sample_light || use_MIS) && render_options.NumLights > 0) {
            float3 pos = payload.pos;
            float3 normal = payload.normal;

            for (int idx = 0; idx < render_options.NumLights; idx++) {
                Light light = Lights[idx];
                light_pick_cdf_sum += EstimateLight(light, pos, normal);
                light_pick_cdf[idx] = light_pick_cdf_sum;
            }

            if (light_pick_cdf_sum > 0.f) {
                uint selected_light_index;
                float selected_light_pdf;
                SelectLight(random_sample.x * light_pick_cdf_sum, light_pick_cdf, selected_light_index, selected_light_pdf);
                selected_light_pdf /= light_pick_cdf_sum;

                Light selected_light = Lights[selected_light_index];
                LightSample light_sample = SampleLight(selected_light, random_sample.yz, pos, normal);
                light_sample.radiance_over_pdf /= selected_light_pdf;
                light_sample.pdf *= selected_light_pdf;

                if (light_sample.pdf > 0.f) {
                    RayDesc light_ray;
                    light_ray.Origin = pos;
                    light_ray.Direction = light_sample.direction;
                    light_ray.TMin = render_options.RayMinBias;
                    light_ray.TMax = light_sample.distance;

                    light_sample.radiance_over_pdf *= TraceVisibilityRay(light_ray);
                }

                if (any(light_sample.radiance_over_pdf) > 0.f && should_accumulate_radiance) {
                    MaterialEval material_eval = EvalMaterial(-ray.Direction, light_sample.direction, payload);
                    float3 light_contrib = light_sample.radiance_over_pdf * material_eval.weight * material_eval.pdf;
                    if (use_MIS) {
                        light_contrib *= MISWeightRobust(light_sample.pdf, material_eval.pdf);
                    }
                    radiance += path_throughput * light_contrib;
                    if (bounce >= 1) {
                        second_bounce_radiance += light_contrib * second_bounce_thp;
                    }
                }
            }
        }

        // Sample Material
        MaterialSample material_sample = SampleMaterial(ray.Direction, payload, random_sample);
        if (material_sample.pdf <= SHADOWY_SMALL_NUMBER || asuint(material_sample.pdf) > 0x7F800000) {
//             radiance = float3(0, 0, 100.f);
            break;
        }

        if (is_camera_ray) {
            restir_sample.second_ray_pdf = material_sample.pdf;
            restir_sample.second_dir = material_sample.direction;
        }

        // Update PathThroughput and Russian Roulette
        float3 next_path_throughput = path_throughput * material_sample.weight;
        path_throughput = next_path_throughput;
        if (bounce >= 1) {
            second_bounce_thp = second_bounce_thp * material_sample.weight;
        }

        // Update Ray
        ray.Origin = payload.pos;
        ray.Direction = material_sample.direction;
        ray.TMin = render_options.RayMinBias;
        ray.TMax = render_options.MaxTraceDistance;

        if ((b_sample_material || use_MIS) && render_options.NumLights > 0) {
            for (uint idx = 0; idx < render_options.NumLights; idx++) {
                Light light = Lights[idx];
                LightHitSample hit_sample = TraceLight(ray, light);
                if (hit_sample.is_miss()) {
                    continue;
                }
                float3 light_contrib = hit_sample.radiance;

                if (use_MIS && light_pick_cdf_sum > 0.f) {
                    float prev_cdf = idx > 0 ? light_pick_cdf[idx - 1] : 0.0;
                    float light_pick_pdf = (light_pick_cdf[idx] - prev_cdf) / light_pick_cdf_sum;
                    light_contrib *= MISWeightRobust(material_sample.pdf, hit_sample.pdf * light_pick_pdf);
                }

                if (render_options.DeferredTraceMaterial) {
                    TraceMaterialState curr_state;
                    curr_state.radiance = light_contrib;
                    curr_state.light_type = light.Type;
                    curr_state.distance = hit_sample.hit_t;
                    trace_material_state[valid_num_trace_light_state++] = curr_state;
                } else {
                    RayDesc light_ray = ray;
                    light_ray.TMax = hit_sample.hit_t;
                    light_contrib *= TraceVisibilityRay(light_ray);
                    if (should_accumulate_radiance) {
                        radiance += path_throughput * light_contrib;
                    }
                    if (bounce >= 1) {
                        second_bounce_radiance += light_contrib * second_bounce_thp;
                    }
                }
            }
        }
    }

    is_valid_sample = any(second_bounce_radiance > 0.f);
    restir_sample.sample_point_Lo = second_bounce_radiance;
    restir_sample.seed = seed;
}
