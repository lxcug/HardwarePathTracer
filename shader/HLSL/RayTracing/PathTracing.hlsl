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
            float vis = TraceShadowRay(payload.pos, light);
            radiance += path_throughput * vis * diffuse_lambert(payload.albedo) * light.Color * light.Intensity;
        }

        if (!is_last_bounce) {
            RayDesc ray;
            ray.Origin = payload.pos;
            ray.Direction = UniformSampleHemisphere(rnd2(seed), payload.normal);  // implies pdf = cos / PI
            ray.TMin = render_options.RayMinBias;
            ray.TMax = 1e3f;  // TODO: TMax as PushConstant
            TraceRay(TLAS, RAY_FLAG_NONE, 0xff, 0, 0, 0, ray, payload);
//             path_throughput /= PI  // path_through * cos(radiance cosine term) reduce with pdf when sampling diffuse material
        }
    }

    return radiance;
}