#pragma once

#include "Payload.hlsl"
#include "../../../src/host_device_shared/Light.h"


float TraceVisibilityRay(in RayDesc ray) {
    RayPayload payload;
    TraceRay(TLAS, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xff, 0, 0, 0, ray, payload);

    return !payload.is_hit;
}

struct LightHitSample {
    float3 radiance;
    float pdf;
    float3 direction;
    float hit_t;

    bool is_hit() {
        return hit_t > 0.f;
    }
    bool is_miss() {
        return hit_t <= 0.f;
    }
};

LightHitSample TraceDirectionalLight(in RayDesc ray, in Light light) {
    LightHitSample light_sample;
    light_sample.radiance = light.Intensity * light.Color;
//     light_sample.pdf = TODO
    light_sample.direction = -normalize(light.Direction);
    light_sample.hit_t = render_options.MaxTraceDistance;

    return light_sample;
}

LightHitSample TracePointLight(in RayDesc ray, in Light light) {
    float radius2 = light.Radius * light.Radius;
//     float3 oc = ray.Origin - light.Position;
//     float b = dot(oc, ray.Direction);
//     float h = radius2 - dot(oc - b * ray.Direction, oc - b * ray.Direction);

    float3 to_light = light.Position - ray.Origin;

    float dis2 = dot(to_light, to_light);
    float3 radiance = float3(0.f, 0.f, 0.f);

    float distance_attenuation = 1.f - min(dis2 / radius2, 1.f);
    radiance = light.Intensity * light.Color * distance_attenuation;


//     if (h > 0) {
//         float t = -b - sqrt(h);
//         if (t > ray.TMin && t < ray.TMax) {  // TODO: Check this

//         }
//     }

    LightHitSample hit_sample;
    hit_sample.radiance = radiance;
    hit_sample.direction = normalize(to_light);
    hit_sample.hit_t = length(to_light);

    return hit_sample;
}

LightHitSample TraceLight(in RayDesc ray, in Light light) {
    switch (light.Type) {
        case LightType::Directional:
            return TraceDirectionalLight(ray, light);
        case LightType::Point:
            return TracePointLight(ray, light);
        default:
            return (LightHitSample)0;
    }
}




