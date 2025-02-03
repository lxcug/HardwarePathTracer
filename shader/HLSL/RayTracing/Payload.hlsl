#pragma once

struct RayPayload
{
    float3 pos;
    float hit_t;

    float3 normal;
    uint instance_id;

    float3 albedo;
    uint triangle_index;

    float3 emissive;
    bool is_hit;

    float3 specular;
    float opacity;

    float roughness;
    float metallic;

    bool is_miss() {
        return !is_hit;
    }
};

struct HitAttribute
{
    float2 bary;
};
