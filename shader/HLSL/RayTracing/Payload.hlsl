struct RayPayload
{
    float3 pos;
    float hit_t;

    float3 normal;
    uint instance_id;

    float3 albedo;
    uint triangle_index;

    bool is_hit;
    bool is_front_face;
};

struct HitAttribute
{
    float2 bary;
};
