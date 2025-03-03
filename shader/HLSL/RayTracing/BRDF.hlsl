#pragma once

#include "Utils.hlsl"

float3 diffuse_lambert(float3 albedo) {
    return albedo * (1 / PI);
}

// NOTE: H and N must in same hemisphere
float GGX_D(float3 H, float3 N, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

float GGX_D_PDF(float3 L, float3 H, float3 N, float roughness) {
    float a = roughness * roughness;
    float k = (a + 1.0) * (a + 1.0) / 8.0;
    float NoL = abs(dot(N, L));
    float G = NoL / (NoL * (1.0 - k) + k);
    float D = GGX_D(H, N, roughness);
    float HoL = abs(dot(H, L));

    return G * D * HoL / NoL;
}

// NOTE: V, L, N must in same hemisphere
float GGX_Smith(float3 V, float3 L, float3 N, float roughness)
{
    float a = roughness * roughness;
    float k = (a + 1.0) * (a + 1.0) / 8.0;

    float NdotV = max(dot(N, V), 0.0);
    float G1_V = NdotV / (NdotV * (1.0 - k) + k);

    float NdotL = max(dot(N, L), 0.0);
    float G1_L = NdotL / (NdotL * (1.0 - k) + k);

    return G1_V * G1_L;
}

float3 Fresnel_Schlick(float cosTheta, float3 F0, float Roughness)
{
    return F0 + (max((1.0 - Roughness).xxx, F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 CookTorranceBRDF(float3 V, float3 L, float3 N, float roughness, float metallic, float3 albedo, float3 F0)
{
    float3 H = normalize(V + L);

    float D = GGX_D(H, N, roughness);
    float G = GGX_Smith(V, L, N, roughness);
    float3 F = Fresnel_Schlick(max(dot(V, H), 0.0f), F0, roughness);

    float NoV = max(dot(N, V), 0.0f);
    float NoL = max(dot(N, L), 0.0f);

    float3 nominator = D * G * F;
    float denominator = 4.0f * NoV * NoL + SHADOWY_SMALL_NUMBER;
    return nominator / denominator;
}

float4 SampleGGXNormal(float2 u, float3 V, float3 N, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float phi = 2.0 * PI * u.x;
    float cosTheta = sqrt((1.0 - u.y) / (1.0 + (a * a - 1.0) * u.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float3 H = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
    float3 tangent, bitangent;
    GetCoordBasis(N, tangent, bitangent);
    H = H.x * tangent + H.y * bitangent + H.z * N;
    float HoV = max(dot(H, V), 0.f);

    float D = GGX_D(H, N, roughness);
    float HoN = max(dot(H, N), 0.f);
	float PDF = D * HoN / (4.0 * HoV + SHADOWY_SMALL_NUMBER);  // Deprecated

    return float4(H, PDF);
}

float4 SampleGGXReflection(float2 u, float3 V, float3 N, float roughness)
{
    float4 H = SampleGGXNormal(u, V, N, roughness);
    return float4(reflect(-V, H.xyz), H.w);
}

float BRDF_PDF(float3 V, float3 L, float3 N, float roughness, float3 F0) {
    float3 H = normalize(V + L);
    float D_pdf = GGX_D_PDF(L, H, N, roughness);
    return D_pdf / max((4 * dot(L, H)), SHADOWY_SMALL_NUMBER);

    // float3 H = normalize(V + L);
    // float D = GGX_D(H, N, roughness);
    // float HoN = max(dot(H, N), 0.f);
    // float HoV = max(dot(H, V), 0.f);
    // float pdf = D * HoN / (4.0 * HoV + SHADOWY_SMALL_NUMBER);
    // return max(pdf, SHADOWY_SMALL_NUMBER);
}

float3 BTDF(float3 V, float3 L, float3 N, float3 H, float roughness, float eta) {
    float3 L_prime = L;
    if (dot(L_prime, H) < 0.f)
        L_prime = -L;

    float D = GGX_D(H, N, roughness);
    float G = GGX_Smith(V, L_prime, N, roughness);
    float HoL = dot(H, L);
    float HoV = dot(H, V);
    float denom = pow(HoL + HoV * eta, 2);
    float NoL = abs(dot(N, L));
    float NoV = abs(dot(N, V));

    return D * G * HoL * HoV / (denom * NoL * NoV);
}

// H and N are in the same hemisphere
float BTDF_PDF(float3 V, float3 L, float3 N, float3 H, float roughness, float eta) {
    float D_pdf = GGX_D_PDF(L, H, N, roughness);
    float HoL = dot(H, L);
    float HoV = dot(H, V);
    float denom = pow(HoL + HoV * eta, 2);
    float dH_dL = abs(HoL) / denom;
    float L_pdf = D_pdf * dH_dL;

    return L_pdf;
}

// [Burley 2012, "Physically-Based Shading at Disney"]
float3 Diffuse_Burley_Disney(float3 DiffuseColor, float3 F0, float Roughness, float NoV, float NoL, float HoL)
{
    float3 FL = Fresnel_Schlick(NoL, F0, Roughness), FV = Fresnel_Schlick(NoV, F0, Roughness);

    float Fd90 = 0.5 + 2 * HoL * HoL * Roughness;
    float3 Fd = lerp(1.0, Fd90, FL) * lerp(1.0, Fd90, FV);

    return Fd * DiffuseColor / PI;
}

float Fresnel_Schlick_Refract(float cosTheta, float F0) {
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float FrDielectric(float cosTheta_i, float eta)
{
    cosTheta_i = clamp(cosTheta_i, -1.0f, 1.0f);

    // Compute Fresnel equations using Snell¡¯s law
    float sin2Theta_i = 1.0f - cosTheta_i * cosTheta_i;
    float sin2Theta_t = sin2Theta_i * eta * eta;

    if (sin2Theta_t >= 1.0f)
        return 1.0f;

    float cosTheta_t = sqrt(1.0f - sin2Theta_t);

    float r_parl = (eta * cosTheta_i - cosTheta_t) / (eta * cosTheta_i + cosTheta_t);
    float r_perp = (cosTheta_i - eta * cosTheta_t) / (cosTheta_i + eta * cosTheta_t);

    return (r_parl * r_parl + r_perp * r_perp) * 0.5f;
}


