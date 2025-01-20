#pragma once

#define SHADOWY_SMALL_NUMBER 1e-6

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

float3 LocalToWorld(in float3 normal, in float3 local_dir) {
    float3 tangent, bitangent;
    GetCoordBasis(normal, tangent, bitangent);
    return local_dir.x * tangent + local_dir.y * bitangent + local_dir.z * normal;
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

float4 UniformSampleCone( float2 E, float CosThetaMax )
{
	float Phi = 2 * PI * E.x;
	float CosTheta = lerp( CosThetaMax, 1, E.y );
	float SinTheta = sqrt( 1 - CosTheta * CosTheta );

	float3 L;
	L.x = SinTheta * cos( Phi );
	L.y = SinTheta * sin( Phi );
	L.z = CosTheta;

	float PDF = 1.0 / ( 2 * PI * (1 - CosThetaMax) );

	return float4( L, PDF );
}

// Same as the function above, but uses SinThetaMax^2 as the parameter
// so that the solid angle can be computed more accurately for very small angles
// The caller is expected to ensure that SinThetaMax2 is <= 1
float4 UniformSampleConeRobust(float2 E, float SinThetaMax2)
{
	float Phi = 2 * PI * E.x;
	// The expression 1-sqrt(1-x) is susceptible to catastrophic cancelation.
	// Instead, use a series expansion about 0 which is accurate within 10^-7
	// and much more numerically stable.
	float OneMinusCosThetaMax = SinThetaMax2 < 0.01 ? SinThetaMax2 * (0.5 + 0.125 * SinThetaMax2) : 1 - sqrt(1 - SinThetaMax2);

	float CosTheta = 1 - OneMinusCosThetaMax * E.y;
	float SinTheta = sqrt(1 - CosTheta * CosTheta);

	float3 L;
	L.x = SinTheta * cos(Phi);
	L.y = SinTheta * sin(Phi);
	L.z = CosTheta;
	float PDF = 1.0 / (2 * PI * OneMinusCosThetaMax);

	return float4(L, PDF);
}

float MISWeightRobust(float Pdf, float OtherPdf) {
	// The straightforward implementation above is prone to numerical overflow and divisions by 0
	// and does not work well with +inf inputs.

	// We want this function to have the following properties:
	//  0 <= w(a,b) <= 1 for all possible positive floats a and b (including 0 and +inf)
	//  w(a, b) + w(b, a) == 1.0

	// The formulation below is much more stable across the range of all possible inputs
	// and guarantees the sum always adds up to 1.0.

	if (Pdf == OtherPdf)
	{
		// Catch potential NaNs from (0,0) and (+inf, +inf)
		return 0.5f;
	}

	// Evaluate the expression using the ratio of the smaller value to the bigger one for greater
	// numerical stability. The math would also work using the ratio of bigger to smaller value,
	// which would underflow less but would make the weights asymmetric. Underflow to 0 is not a
	// bad property to have in rendering application as it ensures more weights are exactly 0
	// which allows some evaluations to be skipped.
	if (OtherPdf < Pdf)
	{
		float x = OtherPdf / Pdf;
		return 1.0 / (1.0 + x * x);
	}
	else
	{
		// this form guarantees the weights add back up to one when arguments are swapped
		float x = Pdf / OtherPdf;
		return 1.0 - 1.0 / (1.0 + x * x);
	}
}

float Luminance( float3 LinearColor )
{
	return dot( LinearColor, float3( 0.3, 0.59, 0.11 ) );
}
