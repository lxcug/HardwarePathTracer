float max(float3 value) {
    return max(value.x, max(value.y, value.z));
}

// Reference: A Survey of Efficient Representations for Independent Unit Vectors
float2 unit_vector_to_octahedron(in float3 dir) {
    dir.xy /= dot(1.f, abs(dir));
    if (dir.z <= 0) {
        if (dir.x > 0.f && dir.y >= 0.f) {
            dir.xy = (1.f - abs(dir.yx));
        } else {
            dir.xy = (1.f - abs(dir.yx));
        }
    }
    return dir.xy;
}

float3 octahedron_to_unit_vector(in float2 uv) {
	float3 dir = float3(uv, 1.f - dot(1.f, abs(uv)));
	if(dir.z < 0.f) {
	    if (dir.x >= 0 && dir.y >= 0) {
            dir.xy = (1.f - abs(dir.yx));
	    } else {
            dir.xy = (1.f - abs(dir.yx)) * -1.f;
	    }
	}
	return normalize(dir);
}

float2 uint_vector_to_hdri_uv(in float3 dir) {
    return float2((PI - atan2(dir.z, dir.x)) / (2 * PI), acos(-dir.y) / PI);
}

float3 sample_sky_texture(in float3 dir) {
    return SkyTexture.SampleLevel(SkyTextureSampler, uint_vector_to_hdri_uv(dir), 0).rgb;
}
