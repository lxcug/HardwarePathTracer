#pragma once

#include "GlobalDefs.hlsl"
#include "../../../src/host_device_shared/Light.h"

#define MAX_SKY_LIGHT_INTENSITY 1e2f


float TraceVisibilityRay(in RayDesc ray) {
    RayPayload payload;
    TraceRay(TLAS, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xff, 0, 0, 0, ray, payload);

    return !payload.is_hit();
}

struct LightHitSample {
    float3 radiance;
    float pdf;
    float hit_t;

    bool is_hit() {
        return hit_t > 0.f;
    }
    bool is_miss() {
        return hit_t <= 0.f;
    }
};

LightHitSample TraceDirectionalLight(in RayDesc ray, in Light light) {
    if (ray.TMax >= render_options.MaxTraceDistance) {
        float sin_theta_max = light.HalfSinAngleOrRange;
        float sin_theta_max2 = pow(sin_theta_max, 2);
        float one_minus_cos_theta_max = sin_theta_max2 < 1e-2f ? sin_theta_max2 * (.5f + .125f * sin_theta_max2) : 1 - sqrt(1 - sin_theta_max2);
        float cos_theta = saturate(dot(ray.Direction, normalize(-light.Direction)));

        // If the ray intersect the directional light cone
        if (1 - cos_theta < one_minus_cos_theta_max) {
            LightHitSample light_sample;
            light_sample.hit_t = render_options.MaxTraceDistance;
            // Hemisphere solid angle 2 * PI
            float solid_angle = 2 * PI * one_minus_cos_theta_max;
            light_sample.pdf = 1.f / solid_angle;
            /*
             * NOTE: For Directional Light, Lo = light.Intensity * light.Color * pdf = light.Intensity * light.Color / solid_angle
             */
            light_sample.radiance = light.Intensity * light.Color / solid_angle;

            return light_sample;
        }
    }

    return (LightHitSample)0;
}

float PointLightAttenuation(in Light light, in float distance2) {
    return 1.f - min(distance2 / pow(light.HalfSinAngleOrRange, 2), 1.f);
}

LightHitSample TracePointLight(in RayDesc ray, in Light light) {
    float radius = light.Radius;
    if (radius == 0.f) {
        return (LightHitSample)0;
    }

    float radius2 = pow(radius, 2);
    float3 oc = ray.Origin - light.Position;
    float b = dot(oc, ray.Direction);
    float h = radius2 - length(oc - b * ray.Direction);

    if (h > 0.f) {
        float t = -b - sqrt(h);
        if (t > ray.TMin && t < ray.TMax) {
            LightHitSample hit_sample;
            float dis2 = dot(oc, oc);
            // Actually / (4 * PI * radius2), 4 is missing for some reason(align with ue)
            float3 power = light.Color * light.Intensity / (PI * radius2);
            float attenuation = PointLightAttenuation(light, dis2);
            hit_sample.radiance = power * attenuation;
            hit_sample.hit_t = t;

            float sin_theta_max2 = saturate(radius2 / dis2);
            float one_minus_cos_theta_max = sin_theta_max2 < 1e-2f ? sin_theta_max2 * (.5f + .125f * sin_theta_max2) : 1 - sqrt(1 - sin_theta_max2);
            float solid_angle = 2 * PI * one_minus_cos_theta_max;
            hit_sample.pdf = 1.f / solid_angle;

            return hit_sample;
        }
    }

    return (LightHitSample)0;
}

LightHitSample TraceSkyLight(in RayDesc ray, in Light light) {
    if (light.Intensity == 0.f) {
        return (LightHitSample)0;
    }

    LightHitSample hit_sample;
    hit_sample.pdf = 1 / (4 * PI);
    hit_sample.radiance = light.Intensity * SkyTexture.SampleLevel(SkyTextureSampler, uint_vector_to_hdri_uv(-ray.Direction), 0).rgb;
    hit_sample.hit_t = render_options.MaxTraceDistance;

    return hit_sample;
}

LightHitSample TraceLight(in RayDesc ray, in Light light) {
    switch (light.Type) {
        case LightType::Directional:
            return TraceDirectionalLight(ray, light);
        case LightType::Point:
            return TracePointLight(ray, light);
        case LightType::Sky:
            return TraceSkyLight(ray, light);
        default:
            return (LightHitSample)0;
    }
}




