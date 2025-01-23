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

float4 UniformSampleHemisphere(in float2 rnd, in float3 normal) {
    float3 local_dir = UniformSampleHemisphere(rnd);
    float3 tangent, bitangent;

    GetCoordBasis(normal, tangent, bitangent);
    return float4(local_dir.x * tangent + local_dir.y * bitangent + local_dir.z * normal, 1.f / (2.f * PI));
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

float4 UniformSampleSphere(float2 E)
{
	float Phi = 2 * PI * E.x;
	float CosTheta = 1 - 2 * E.y;
	float SinTheta = sqrt( 1 - CosTheta * CosTheta );

	float3 H;
	H.x = SinTheta * cos( Phi );
	H.y = SinTheta * sin( Phi );
	H.z = CosTheta;

	float PDF = 1.0 / (4 * PI);

	return float4( H, PDF );
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

float2 InverseEquiAreaSphericalMapping(float3 Direction)
{
	float3 AbsDir = abs(Direction);
	float R = sqrt(1 - AbsDir.z);
	float Epsilon = 5.42101086243e-20; // 2^-64 (this avoids 0/0 without changing the rest of the mapping)
	float x = min(AbsDir.x, AbsDir.y) / (max(AbsDir.x, AbsDir.y) + Epsilon);

	// Coefficients for 6th degree minimax approximation of atan(x)*2/pi, x=[0,1].
	const float t1 = 0.406758566246788489601959989e-5f;
	const float t2 = 0.636226545274016134946890922156f;
	const float t3 = 0.61572017898280213493197203466e-2f;
	const float t4 = -0.247333733281268944196501420480f;
	const float t5 = 0.881770664775316294736387951347e-1f;
	const float t6 = 0.419038818029165735901852432784e-1f;
	const float t7 = -0.251390972343483509333252996350e-1f;

	// Polynomial approximation of atan(x)*2/pi
	float Phi = t6 + t7 * x;
	Phi = t5 + Phi * x;
	Phi = t4 + Phi * x;
	Phi = t3 + Phi * x;
	Phi = t2 + Phi * x;
	Phi = t1 + Phi * x;

	Phi = (AbsDir.x < AbsDir.y) ? 1 - Phi : Phi;
	float2 UV = float2(R - Phi * R, Phi * R);
	UV = (Direction.z < 0) ? 1 - UV.yx : UV;
	UV = asfloat(asuint(UV) ^ (asuint(Direction.xy) & 0x80000000u));
	return UV * 0.5 + 0.5;
}

