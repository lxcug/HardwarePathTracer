#pragma once

#include "Utils.hlsl"

float3 diffuse_lambert(float3 albedo) {
    return albedo * (1 / PI);
}

float GGX_D(float3 H, float3 N, float roughness)
{
    float NdotH = max(dot(N, H), 0.0f);
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (alpha2 - 1.0f) + 1.0f;
    return alpha2 / (PI * denom * denom);
}

float GGX_Smith(float3 V, float3 H, float3 N, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotH = max(dot(N, H), 0.0f);
    float VdotH = max(dot(V, H), 0.0f);

    float alpha = roughness * roughness;
    float k = (alpha + 1.0f) / 2.0f;

    float G1 = NdotV / (NdotV * (1.0f - k) + k);
    float G2 = NdotH / (NdotH * (1.0f - k) + k);

    return G1 * G2;
}

float Fresnel_Schlick(float cosTheta, float F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float3 CookTorranceBRDF(float3 V, float3 L, float3 N, float roughness, float F0)
{
    float3 H = normalize(V + L);

    float D = GGX_D(H, N, roughness);
    float G = GGX_Smith(V, H, N, roughness);
    float F = Fresnel_Schlick(max(dot(V, H), 0.0f), F0);

    float3 nominator = D * G * F;
    float denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 1e-5;
    return max(nominator / denominator, SHADOWY_SMALL_NUMBER);
}

float3 SampleGGX(float2 u, float3 V, float3 N, float roughness)
{
    float alpha = roughness * roughness;

    float phi = 2.0f * PI * u.x;
    float cosTheta = sqrt((1.0f - u.y) / (1.0f + (alpha * alpha - 1.0f) * u.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 H = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
    float3 tangent, bitangent;
    GetCoordBasis(N, tangent, bitangent);
    H = H.x * tangent + H.y * bitangent + H.z * N;

    float3 T = normalize(2.0f * dot(V, H) * H - V);

    return T;
}

float GGX_PDF(float3 H, float3 N, float roughness)
{
    float NdotH = max(dot(N, H), 0.0f);
    float D = GGX_D(H, N, roughness);

    return max(D / (4.0f * NdotH), SHADOWY_SMALL_NUMBER);
}

float BRDF_PDF(float3 V, float3 L, float3 N, float roughness, float F0)
{
    float3 H = normalize(V + L);
    float D = GGX_D(H, N, roughness);
    float G_v = GGX_Smith(V, H, N, roughness);
    float G_l = GGX_Smith(L, H, N, roughness);
    float F = Fresnel_Schlick(max(dot(V, H), 0.0f), F0);

    float pdf = D * G_v * G_l * F / (4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f));

    return max(pdf, SHADOWY_SMALL_NUMBER);
}
