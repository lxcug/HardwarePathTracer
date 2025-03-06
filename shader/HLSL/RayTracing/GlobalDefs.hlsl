#pragma once


struct RayPayload
{
    uint seed;
    uint instance_id;
    uint primitive_id;
    float hit_t;
    float2 bary;
    float4x3 obj2world;
    float4x3 world2obj;

    bool is_hit() {
        return hit_t > 0.f;
    }
};

struct HitAttribute
{
    float2 bary;
};

struct PathTracingReturn {
    float3 radiance;
    float hit_t;
    bool hit_sky;

    bool is_hit() {
        return hit_t > 0.f;
    }
};

/*
 * No need to align with 4B, since this structured is resolved in shaders.
 * Stores material after texture sampling.
 */
struct ShaderMaterial {
    float3 base_color;
    float transmission;
    float3 emission;

    float3 sheen;
    float3 sheen_tint;
    float clearcoat;
    float clearcoat_roughness;
    float3 subsurface_color;
    float subsurface;
    float anisotropy;
    float ax;
    float ay;

    float specular_tint;
    float specular;

    float roughness;
    float metallic;
    float ior;

    float opacity;
    uint alpha_mode;
    float alpha_cutoff;
    int two_sided;
};

/*
 * Used for AnyHit Shader Ignore hit, reduce texture query when resolving
 */
struct AnyHitMaterial {
    float3 base_color;
    float opacity;
    int two_sided;
    uint alpha_mode;
    float alpha_cutoff;
};

/*
 * Record Ray Path Data
 */
struct PathState {
    float3 pos;
    float3 normal;
    float3 face_front_normal;
    float3 tangent;
    float3 bitangent;
    float2 uv;
    float eta;

    bool is_specular_bounce;
    bool is_subsurface;

    uint64_t mat_addr;  // for query InputMaterial, init by ResolvePathStateGeometry
    ShaderMaterial mat;  // store resolved ShaderMaterial
};




