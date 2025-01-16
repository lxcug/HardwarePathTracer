float3 UniformSampleHemisphere(in float rnd1, in float rnd2) {
    float3 dir = float3(cos(2 * PI * rnd1) * sqrt(1 - rnd2), sin(2 * PI * rnd1) * sqrt(1 - rnd2), sqrt(rnd2));
    return dir;
}

float3 UniformSampleHemisphere(in float2 rnd) {
    return UniformSampleHemisphere(rnd.x, rnd.y);
}

void GetCoordBasis(in float3 normal, out float3 tangent, out float3 bitangent) {
    float3 z  = normal;
    const float yz = -z.y * z.z;
    bitangent = normalize(((abs(z.z) > 0.99999f) ? float3(-z.x * z.y, 1.0f - z.y * z.y, yz) : float3(-z.x * z.z, yz, 1.0f - z.z * z.z)));

    tangent = cross(bitangent, z);
}

float3 UniformSampleHemisphere(in float rnd1, in float rnd2, in float3 normal) {
    float3 local_dir = UniformSampleHemisphere(rnd1, rnd2);
    float3 tangent, bitangent;

    GetCoordBasis(normal, tangent, bitangent);
    return local_dir.x * tangent + local_dir.y * bitangent + local_dir.z * normal;
}

float3 UniformSampleHemisphere(in float2 rnd, in float3 normal) {
    float3 local_dir = UniformSampleHemisphere(rnd);
    float3 tangent, bitangent;

    GetCoordBasis(normal, tangent, bitangent);
    return local_dir.x * tangent + local_dir.y * bitangent + local_dir.z * normal;
}


/*
 * Distance Based AO
 * Return a float in [0, 1], 0 represents occlusion, 1 represents dis-occlusion
 */
float TraceAORay(in float3 origin, in float3 direction, float2 rnd) {
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = render_options.RayMinBias;
    ray.TMax = render_options.AORayLength;

    RayPayload payload;
    TraceRay(TLAS, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xff, 0, 0, 0, ray, payload);
    if (!payload.is_hit) {
        return render_options.AORayLength;
    }
    return payload.hit_t;
}


/*
 * Return 1 for dis-occlusion, 0 for occlusion
 */
float TraceShadowRay(in float3 origin, in Light light) {
    RayDesc ray;
    ray.Origin = origin;
    ray.TMin = render_options.RayMinBias;
    if (light.Type == LightType::Directional) {
        ray.Direction = -normalize(light.Direction);
        ray.TMax = render_options.MaxTraceDistance;
    } else {
        float3 ToLight = light.Position - origin;
        ray.Direction = normalize(ToLight);
        ray.TMax = length(ToLight);
    }

    RayPayload payload;
    TraceRay(TLAS, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xff, 0, 0, 0, ray, payload);
    return !payload.is_hit;
}

float TraceShadowRay(in float3 origin, in Light light, out float3 ray_direction) {
    RayDesc ray;
    ray.Origin = origin;
    ray.TMin = render_options.RayMinBias;
    if (light.Type == LightType::Directional) {
        ray.Direction = -normalize(light.Direction);
        ray.TMax = render_options.MaxTraceDistance;
    } else {
        float3 ToLight = light.Position - origin;
        ray.Direction = normalize(ToLight);
        ray.TMax = length(ToLight);
    }

    ray_direction = ray.Direction;

    RayPayload payload;
    TraceRay(TLAS, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xff, 0, 0, 0, ray, payload);
    return !payload.is_hit;
}

void TraceLight(in float3 origin, in float3 normal, in Light light, out float3 radiance) {
    float distance_attenuation = 1.f;
    float3 ray_direction;
    if (light.Type == LightType::Point || light.Type == LightType::Spot) {
        float3 to_light = light.Position - origin;
        float dis2 = dot(to_light, to_light);
        float radius2 = light.Radius * light.Radius;
        if (dis2 > radius2) {
            radiance = float3(0.f, 0.f, 0.f);
            return;
        }
        distance_attenuation = 1.f - min(dis2 / radius2, 1.f);
    }

    float vis = TraceShadowRay(origin, light, ray_direction);

    radiance = vis * light.Intensity * light.Color * dot(normal, ray_direction) * distance_attenuation;
}

