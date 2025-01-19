#pragma once

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

// TODO: Move Sampling Code to a new header file
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

float4 CosineSampleHemisphere(in float2 rnd) {
    float phi = 2 * PI * rnd.x;
    float cos_theta = sqrt(rnd.y);
    float sin_theta = sqrt(1 - cos_theta * cos_theta);

    float3 dir_local = {
        sin_theta * cos(phi),
        sin_theta * sin(phi),
        cos_theta
    };

    float pdf = cos_theta / PI;

    return float4(dir_local, pdf);
}

float4 CosineSampleHemisphere(in float2 rnd, in float3 normal) {
    float phi = 2 * PI * rnd.x;
    float cos_theta = sqrt(rnd.y);
    float sin_theta = sqrt(1 - cos_theta * cos_theta);

    float3 dir_local = {
        sin_theta * cos(phi),
        sin_theta * sin(phi),
        cos_theta
    };

    float3 tangent, bitangent;
    GetCoordBasis(normal, tangent, bitangent);

    float3 dir_world = dir_local.x * tangent + dir_local.y * bitangent + dir_local.z * normal;

    float pdf = cos_theta / PI;

    return float4(dir_world, pdf);
}
