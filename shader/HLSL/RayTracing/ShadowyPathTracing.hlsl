#pragma once

#include "RayTracingCommon.hlsl"
#include "PathStateResolve.hlsl"
#include "LightSampling.hlsl"
#include "ShadowyMaterialSampling.hlsl"

#define MAX_LIGHTS 64

struct DeferredTraceMaterialRayState {
    LightType light_type;
    float3 radiance;
    float distance;
};

/*
 * Sample Directional/Point/Spot/Sky Lights and eval light contribution
 */
float3 NextEventEstimation(
    in PathState state,
    in RayDesc ray,
    inout uint seed,
    inout float light_pick_cdf_sum,
    inout float light_pick_cdf[MAX_LIGHTS],
    bool mis
) {
    float3 light_contrib = 0.f;

    // power heuristic light sampling
    for (int idx = 0; idx < render_options.NumLights; idx++) {
        Light light = Lights[idx];
        light_pick_cdf_sum += EstimateLight(light, state.pos, state.face_front_normal);
        light_pick_cdf[idx] = light_pick_cdf_sum;
    }

    if (light_pick_cdf_sum > 0.f) {
        uint selected_light_index;
        float selected_light_pdf;
        SelectLight(rnd(seed) * light_pick_cdf_sum, light_pick_cdf, selected_light_index, selected_light_pdf);
        selected_light_pdf /= light_pick_cdf_sum;

        Light selected_light = Lights[selected_light_index];
        LightSample light_sample = SampleLight(selected_light, rnd2(seed), state.pos, state.face_front_normal);
        light_sample.radiance_over_pdf /= selected_light_pdf;
        light_sample.pdf *= selected_light_pdf;
        light_sample.radiance_over_pdf *= (dot(light_sample.direction, state.face_front_normal) > 0.f || state.is_subsurface);  // IMPORTANT

        if (any(light_sample.radiance_over_pdf) > 0.f) {
            RayDesc light_ray;
            light_ray.Origin = state.pos;
            light_ray.Direction = light_sample.direction;
            light_ray.TMin = render_options.RayMinBias;
            light_ray.TMax = light_sample.distance;
            light_sample.radiance_over_pdf *= TraceVisibilityRay(light_ray);
            ShadowyMaterialEval mat_eval =
                ShadowyEvalMaterial(state, -ray.Direction, light_sample.direction, state.face_front_normal);
            light_contrib = light_sample.radiance_over_pdf * mat_eval.bsdf *
                abs(dot(light_sample.direction, state.face_front_normal));

            if (mis) {
                light_contrib *= MISWeightRobust(light_sample.pdf, mat_eval.pdf);
            }
        }
    }

    return light_contrib;
}


void PathTracing(
    in float3 origin,
    in float3 direction,
    inout uint seed,
    inout PathTracingReturn pt_ret
) {
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = normalize(direction);
    ray.TMin = render_options.RayMinBias;
    ray.TMax = render_options.MaxTraceDistance;

    float3 radiance = 0.f;
    float3 thp = 1.f;
    float3 absorption = 0.f;
    RayPayload payload;

    pt_ret.hit_sky = false;
    pt_ret.hit_t = -1.f;

    bool include_emission = true;
    bool mis = true;
    bool deferred_trace_material_ray = true;

    PathState state;

    float light_pick_cdf[MAX_LIGHTS];
    DeferredTraceMaterialRayState trace_material_ray_state[MAX_LIGHTS];
    uint valid_num_trace_light_state = 0;

    [loop]
    for (int bounce = 0; bounce < render_options.Bounce; bounce++) {
        bool is_camera_ray = bounce == 0;

        TraceRay(TLAS, RAY_FLAG_NONE, 0xff, 0, 0, 0, ray, payload);

        if (mis && deferred_trace_material_ray) {
            for (int idx = 0; idx < valid_num_trace_light_state; idx++) {
                DeferredTraceMaterialRayState curr_state = trace_material_ray_state[idx];
                if (!payload.is_hit() || payload.hit_t >= curr_state.distance) {
                    radiance += curr_state.radiance;
                }
            }
            valid_num_trace_light_state = 0;
        }

        if (!payload.is_hit()) {
            if (is_camera_ray) {
                pt_ret.hit_sky = true;
                radiance += thp * SkyTexture.SampleLevel(SkyTextureSampler, uint_vector_to_hdri_uv(-ray.Direction), 0).rgb;
            }
            break;
        }

        if (is_camera_ray) {
            pt_ret.hit_t = payload.hit_t;
            pt_ret.instance_id = payload.instance_id;
        }

        ResolvePathStateGeometry(state, ray, payload);
        ResolvePathStateMaterial(state, ray);

        if (render_options.DebugMode == DEBUG_BASE_COLOR) {
            radiance = state.mat.base_color;
            break;
        } else if (render_options.DebugMode == DEBUG_NORMAL) {
            radiance = state.normal * .5f + .5f;
            break;
        } else if (render_options.DebugMode == DEBUG_ROUGHNESS) {
            radiance = state.mat.roughness.xxx;
            break;
        } else if (render_options.DebugMode == DEBUG_METALLIC) {
            radiance = state.mat.metallic.xxx;
            break;
        } else if (render_options.DebugMode == DEBUG_UV) {
            radiance = state.uv.xyx;
            break;
        } else if (render_options.DebugMode == DEBUG_EMISSION) {
            radiance = state.mat.emission;
            break;
        } else if (render_options.DebugMode == DEBUG_INSTANCE_ID) {
            radiance = payload.instance_id / 5000.f;
            break;
        } else if (render_options.DebugMode == DEBUG_OPACITY) {
            radiance = state.mat.opacity;
            break;
        }

        if (include_emission) {
            radiance += thp * state.mat.emission * render_options.EmissionIntensity;
        }
//         thp *= exp(-absorption * payload.hit_t);

        float light_pick_cdf_sum = 0.f;
        radiance += thp * NextEventEstimation(state, ray, seed, light_pick_cdf_sum, light_pick_cdf, mis);

        ShadowyMaterialSample mat_sample = ShadowySampleMaterial(state, -ray.Direction,
            state.face_front_normal, seed);

        if (mat_sample.pdf < 0.f) {
            break;
        }

//         if (dot(state.face_front_normal, mat_sample.direction) < 0.f) {
//             absorption =
//         }

        float3 next_path_thp = thp * mat_sample.bsdf *
            abs(dot(state.face_front_normal, mat_sample.direction)) / mat_sample.pdf;
        float continue_prob = sqrt(max(next_path_thp) / max(thp));
        if (continue_prob < 1.f) {
            if (rnd(seed) >= continue_prob) {
                break;
            }
            thp = next_path_thp / continue_prob;
        } else {
            thp = next_path_thp;
        }

        ray.Origin = state.pos;
        ray.Direction = mat_sample.direction;

        if (mis && render_options.NumLights > 0) {
            for (uint idx = 0; idx < render_options.NumLights; idx++) {
                Light light = Lights[idx];
                LightHitSample hit_sample = TraceLight(ray, light);
                if (hit_sample.is_miss()) {
                    continue;
                }
                float3 light_contrib = thp * hit_sample.radiance;

                if (mis && light_pick_cdf_sum > 0.f) {
                    float prev_cdf = idx > 0 ? light_pick_cdf[idx - 1] : 0.0;
                    float light_pick_pdf = (light_pick_cdf[idx] - prev_cdf) / light_pick_cdf_sum;
                    light_contrib *= MISWeightRobust(mat_sample.pdf, hit_sample.pdf * light_pick_pdf) *
                        (dot(-ray.Direction, state.face_front_normal) > 0.f || state.is_subsurface);  // IMPORTANT
                }

                if (any(light_contrib > 0.f)) {
                    if (deferred_trace_material_ray) {
                        DeferredTraceMaterialRayState curr_state;
                        curr_state.radiance = light_contrib;
                        curr_state.light_type = light.Type;
                        curr_state.distance = hit_sample.hit_t;
                        trace_material_ray_state[valid_num_trace_light_state++] = curr_state;
                    } else {
                        RayDesc light_ray = ray;
                        light_ray.TMax = hit_sample.hit_t;
                        light_contrib *= TraceVisibilityRay(light_ray);
                        radiance += light_contrib;
                    }
                }
            }
        }
    }

    /*
     * Remove fireflies
     * Show different behavior for NEE and Sample Material Only.
     * Since Sample Material get small radiance for most case(expect only a sky light in the scene),
     * when hit light, the radiance will be very big and will be clamped to fire_fly_clamp_threshold,
     * the time accumulated radiance value will be small which we don't expect.
     * When use MIS, the problem will be cancelled, since with MIS, each dispatched ray will get a quite
     * smooth radiance value(it just like each frame we get quite smooth radiance value, little fireflies),
     * so the time accumulated radiance will quite reasonable.
     * Well, if use sample light only, some scenario we hardly sample a good sky light direction, so
     * the clampped result will be cause the same issue.
     * So here, we only apply firefly clamp for MIS.
     */
    bool enable_caustics = false;
    float lum = Luminance(radiance);
    const float fire_fly_clamp_threshold = render_options.FireFlyThreshold;
    if(mis && !enable_caustics && lum > fire_fly_clamp_threshold)
    {
        radiance *= fire_fly_clamp_threshold / lum;
    }

    pt_ret.radiance = radiance;
}
