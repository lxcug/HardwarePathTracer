/*
 * References:
 *     nv best ray tracing practice : https://developer.nvidia.com/blog/best-practices-using-nvidia-rtx-ray-tracing/
 *     nv ray tracing : https://github.com/nvpro-samples/vk_raytrace
 *     UCSD Disney Principled BSDF : https://cseweb.ucsd.edu/~tzli/cse272/wi2024/
 *     Disney Principled BRDF :
 *         https://media.disneyanimation.com/uploads/production/publication_asset/48/asset/s2012_pbs_disney_brdf_notes_v3.pdf
 *         https://github.com/wdas/brdf
 *     PBRT v4: https://pbr-book.org/4ed/contents
 *
 * Disney BXDF is made of 5 components:
 *     diffuse lobe : modeling base diffuse reflection
 *     specular lobe : modeling specular highlights
 *     clearcoat lobe : modeling heavy tail of specular
 *     sheen lobe : modeling retro-reflection
 *     glass lobe: modeling transmission
 */

// ----------------------------------------------------------------------------------------------

#pragma once

float GTR1(
    float NoH,
    float a
) {
    if(a >= 1.f) {
        return 1.f / PI;
    }

    float a2 = a * a;
    float t = 1.f + (a2 - 1.f) * NoH * NoH;
    return (a2 - 1.f) / (PI * log(a2) * t);
}

float GTR2(
    float NoH,
    float a
) {
    float a2 = a * a;
    float t = 1.f + (a2 - 1.f) * NoH * NoH;

    return a2 / (PI * t * t);
}

float GTR2_aniso(
    float NoH,
    float HoX,
    float HoY,
    float ax,
    float ay
)
{
    float a = HoX / ax;
    float b = HoY / ay;
    float c = a * a + b * b + NoH * NoH;

    return 1.f / (PI * ax * ay * c * c);
}

/*
 * Geometry Shadowing or Masking Term
 */
float GGX_Smith(
    float NoV,
    float alpha_G  // roughness actually, named this for align with Disney Principled BRDF source code
) {
    float a = alpha_G * alpha_G;
    float b = NoV * NoV;

    return 1.f / (NoV + sqrt(a + b - a * b));
}

float GGX_Smith_aniso(
    float NoV,
    float VoX,
    float VoY,
    float ax,
    float ay
) {
  float a = VoX * ax;
  float b = VoY * ay;
  float c = NoV;

  return 1.f / (NoV + sqrt(a * a + b * b + c * c));
}

/*
 * Fresnel reflection index for dielectric
 */
float Fresnel_Dielectric(
    float cos_theta_i,
    float eta
) {
    float sin2_theta_t = eta * eta * (1.f - cos_theta_i * cos_theta_i);

    // total reflection
    if (sin2_theta_t > 1.f) {
        return 1.f;
    }

    float cos_theta_t = sqrt(1.f - sin2_theta_t);
    float r_parl = (eta * cos_theta_t - cos_theta_i) / (eta * cos_theta_t + cos_theta_i);
    float r_perp = (eta * cos_theta_i - cos_theta_t) / (eta * cos_theta_i + cos_theta_t);

    return (r_parl * r_parl + r_perp * r_perp) * .5f;
}

/*
 * Fresnel Schlick Approximation
 */
float Fresnel_Schlick(
    float u
) {
    float m = clamp(1.f - u, 0.f, 1.f);
    float m2 = m * m;
    return m2 * m2 * m;  // m^5
}

/*
 * V, H, and N are in the same hemisphere, while L is not
 */
float3 EvalDielectricRefraction(
    in PathState state,
    in float3 V,
    in float3 L,
    in float3 N,
    in float3 H,
    inout float pdf
) {
    float D = GTR2(dot(N, H), state.mat.roughness);
    float F = Fresnel_Dielectric(abs(dot(V, H)), state.eta);
    float G = GGX_Smith(abs(dot(N, L)), state.mat.roughness) * GGX_Smith(dot(N, V), state.mat.roughness);

    float denom_sqrt = dot(L, H) * state.eta + dot(V, H);
    pdf = max(D * (1.f - F) * dot(N, H) * abs(dot(L, H)) / (denom_sqrt * denom_sqrt), SHADOWY_SMALL_NUMBER);

    float3 bsdf = state.mat.base_color * D * (1.f - F) * G * abs(dot(V, H)) * abs(dot(L, H)) *
        4.f * state.eta * state.eta / (denom_sqrt * denom_sqrt);
    return bsdf;
}

float3 EvalDielectricReflection(
    in PathState state,
    in float3 V,
    in float3 L,
    in float3 N,
    in float3 H,
    inout float pdf
) {
    float NoL = dot(N, L);
    if (NoL < 0.f) {
        return 0.f;
    }

    float NoH = dot(N, H);
    float D = GTR2(NoH, state.mat.roughness);
    float F = Fresnel_Dielectric(dot(V, H), state.eta);
    float G = GGX_Smith(NoL, state.mat.roughness) * GGX_Smith(dot(N, V), state.mat.roughness);

    pdf = max(D * F * NoH / (4.f * dot(V, H)), SHADOWY_SMALL_NUMBER);

    float3 brdf = state.mat.base_color * D * F * G;
    return brdf;
}

float3 EvalSubsurface(
    in PathState state,
    in float3 V,
    in float3 L,
    in float3 N,
    inout float pdf
) {
    pdf = 1.f / (2 * PI);

    float FL = Fresnel_Schlick(dot(N, L));
    float FV = Fresnel_Schlick(dot(N, V));
    float Fd = (1.f - .5f * FL) * (1.f - .5f * FV);

    float3 brdf = sqrt(state.mat.base_color) * state.mat.subsurface * (1.f / PI) * Fd *
        (1.f - state.mat.metallic) * (1.f - state.mat.transmission);
    return brdf;
}

float3 EvalDiffuse(
    in PathState state,
    in float3 Csheen,
    in float3 V,
    in float3 L,
    in float3 N,
    in float3 H,
    inout float pdf
) {
    float NoL = dot(N, L);
    if (NoL < 0.f) {
        return 0.f;
    }

    pdf = max(NoL / PI, SHADOWY_SMALL_NUMBER);

    float NoV = dot(N, V);
    float HoL = dot(H, L);
    float FL = Fresnel_Schlick(NoL);
    float FV = Fresnel_Schlick(NoV);
    float FH = Fresnel_Schlick(HoL);
    float Fd90 = .5f + 2.f * HoL * HoL * state.mat.roughness;
    float Fd = lerp(1.f, Fd90, FL) * lerp(1.f, Fd90, FV);
    float3 Fsheen = FH * state.mat.sheen * Csheen;

    float3 brdf = ((1.f / PI) * Fd * (1.f - state.mat.subsurface) * state.mat.base_color + Fsheen) *
        (1.f - state.mat.metallic);
    return brdf;
}

float3 EvalSpecular(
    in PathState state,
    in float3 Cspec0,
    in float3 V,
    in float3 L,
    in float3 N,
    in float3 H,
    inout float pdf
) {
    float NoL = dot(N, L);
    if (NoL < 0.f) {
        return 0.f;
    }

    float NoH = dot(N, H);
    float HoV = dot(H, V);
    float HoL = dot(H, L);
    float NoV = dot(N, V);
    float D = GTR2_aniso(NoH, dot(H, state.tangent), dot(H, state.bitangent), state.mat.ax, state.mat.ay);

    pdf = max(D * NoH / (4.f * HoV), SHADOWY_SMALL_NUMBER);

    float FH = Fresnel_Schlick(HoL);
    float3 F = lerp(Cspec0, 1.f, FH);
    float G = GGX_Smith_aniso(NoL, dot(L, state.tangent), dot(L, state.bitangent), state.mat.ax, state.mat.ay) *
        GGX_Smith_aniso(NoV, dot(V, state.tangent), dot(V, state.bitangent), state.mat.ax, state.mat.ay);

    float3 brdf = D * F * G;
    return brdf;
}

float3 EvalClearcoat(
    in PathState state,
    in float3 V,
    in float3 L,
    in float3 N,
    in float3 H,
    inout float pdf
) {
    float NoL = dot(N, L);
    if (NoL < 0.f) {
        return 0.f;
    }

    float NoH = dot(N, H);
    float HoV = dot(H, V);
    float NoV = dot(N, V);
    float D = GTR2(NoH, lerp(1e-2f, .2f, state.mat.clearcoat_roughness));

    pdf = max(D * NoH / (4.f * HoV), SHADOWY_SMALL_NUMBER);

    float FH = Fresnel_Schlick(dot(H, L));
    float F = lerp(0.04f, 1.f, FH);
    float G = GGX_Smith(NoL, .25f) * GGX_Smith(NoV, .25f);

    float3 brdf = .25f * state.mat.clearcoat * D * F * G;
    return brdf;
}

float3 UniformSampleHemisphere(float2 u)
{
  float r = sqrt(max(0.f, 1.f - u.x * u.x));
  float phi = 2.f * PI * u.y;

  return float3(r * cos(phi), r * sin(phi), u.x);
}

float4 CosSampleHemisphere(in float2 rnd) {
    float phi = 2 * PI * rnd.x;
    float cos_theta = sqrt(rnd.y);
    float sin_theta = sqrt(1 - cos_theta * cos_theta);

    float3 dir_local = {
        sin_theta * cos(phi),
        sin_theta * sin(phi),
        cos_theta
    };

    float pdf = max(cos_theta / PI, SHADOWY_SMALL_NUMBER);

    return float4(dir_local, pdf);
}

float3 ImportanceSampleGTR1(
    float roughness,
    float2 u
) {
    float a  = max(roughness, 1e-3f);
    float a2 = a * a;

    float phi = 2 * PI * u.x;

    float cos_theta = sqrt((1.f- pow(a2, 1.f - u.y)) / (1.f - a2));
    float sin_theta = sqrt(1.f - cos_theta * cos_theta);
    float sin_phi = sin(phi);
    float cos_phi = cos(phi);

    return float3(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
}


float3 ImportanceSampleGTR2(
    float roughness,
    float2 u
) {
    float a = max(roughness, 1e-3f);
    float cos_theta = sqrt((1.f - u.y) / (1.f + (a * a - 1.f) * u.y));
    float sin_theta = sqrt(1.f - (cos_theta * cos_theta));
    float phi = 2 * PI * u.x;
    float sin_phi = sin(phi);
    float cos_phi = cos(phi);

    return float3(sin_theta * cos_phi, sin_theta * sin_phi, cos_theta);
}

float3 ImportanceSampleGTR2_aniso(
    float ax,
    float ay,
    float2 u
) {
    float phi = 2 * PI * u.x;

    float sin_phi = ay * sin(phi);
    float cos_phi = ax * cos(phi);
    float tan_theta = sqrt(u.y / (1.f - u.x));

    return float3(tan_theta * cos_phi, tan_theta * sin_phi, 1.f);
}


void ShadowyEvalDisneyMaterial(
    in PathState state,
    in float3 V,
    in float3 L,
    in float3 N,
    inout float3 out_bxdf,
    inout float out_pdf
) {
    float3 H;
    float NoL = dot(N, L);
    if (NoL < 0.f) {  // refraction half vector
        H = normalize(L * (1.f / state.eta) + V);
    } else {  // reflection half vector
       H = normalize(L + V);
    }

    // force H and N in same hemisphere
    if (dot(N, H) < 0.f) {
        H = -H;
    }

    float diffuse_ratio = .5f * (1.f - state.mat.metallic);
    float primary_spec_ratio = 1.f / (1.f + state.mat.clearcoat);
    float trans_weight = (1.f - state.mat.metallic) * state.mat.transmission;

    float3 brdf = 0.f;
    float brdf_pdf = 0.f;
    float3 bsdf = 0.f;
    float bsdf_pdf = 0.f;

    if (trans_weight > 0.f) {
        if (NoL < 0.f) {  // dielectric refraction
            bsdf = EvalDielectricRefraction(state, V, L, N, H, bsdf_pdf);
        } else {  // dielectric reflection
            bsdf = EvalDielectricReflection(state, V, L, N, H, bsdf_pdf);
        }
    }

    float m_pdf = 0.f;
    if (trans_weight < 1.f) {
        if (dot(L, N) < 0.f) {  // subsurface
            if (state.mat.subsurface > 0.f) {
                brdf = EvalSubsurface(state, V, L, N, m_pdf);
                brdf_pdf = m_pdf * state.mat.subsurface * diffuse_ratio;
            }
        } else {  // brdf
            float3 Cdlin = state.mat.base_color;
            float Cdlum = .3f * Cdlin[0] + .6f * Cdlin[1] + .1f * Cdlin[2];  // luminance approx.

            float3 Ctint = Cdlum > 0.f ? Cdlin / Cdlum : 1.f; // normalize lum. to isolate hue + sat
            float3 Cspec0 = lerp(state.mat.specular * .08f *
                lerp(1.f, Ctint, state.mat.specular_tint), Cdlin, state.mat.metallic);
            float3 Csheen = state.mat.sheen_tint;  // lerp(1.f, Ctint, state.mat.sheen_tint);

            // diffuse
            brdf += EvalDiffuse(state, Csheen, V, L, N, H, m_pdf);
            brdf_pdf += m_pdf * (1.f - state.mat.subsurface) * diffuse_ratio;

            // specular
            brdf += EvalSpecular(state, Cspec0, V, L, N, H, m_pdf);
            brdf_pdf += m_pdf * primary_spec_ratio * (1.f - diffuse_ratio);

            // clearcoat
            brdf += EvalClearcoat(state, V, L, N, H, m_pdf);
            brdf_pdf += m_pdf * (1.f - primary_spec_ratio) * (1.f - diffuse_ratio);
        }
    }

    out_bxdf = lerp(brdf, bsdf, trans_weight);
    out_pdf = lerp(brdf_pdf, bsdf_pdf, trans_weight);
}

void ShadowySampleDisneyMaterial(
    in PathState state,
    in float3 V,
    in float3 N,
    inout uint seed,
    inout float3 out_dir,
    inout float3 out_bxdf,
    inout float out_pdf
) {
    state.is_subsurface = false;
    float3 bxdf = 0.f;

    float2 random2 = rnd2(seed);

    float diffuse_ratio = .5f * (1.f - state.mat.metallic);
    float primary_spec_ratio = 1.f / (1.f + state.mat.clearcoat);
    float trans_weight = (1.f - state.mat.metallic) * state.mat.transmission;

    float3 Cdlin = state.mat.base_color;
    float Cdlum = .3f * Cdlin[0] + .6f * Cdlin[1] + .1f * Cdlin[2];  // luminance approx.

    float3 Ctint = Cdlum > 0.f ? Cdlin / Cdlum : 1.f; // normalize lum. to isolate hue + sat
    float3 Cspec0 = lerp(state.mat.specular * .08f *
        lerp(1.f, Ctint, state.mat.specular_tint), Cdlin, state.mat.metallic);
    float3 Csheen = state.mat.sheen_tint;  // lerp(1.f, Ctint, state.mat.sheen_tint);

    float3 L;
    if (rnd(seed) < trans_weight) {  // sample bsdf
        float3 H = ImportanceSampleGTR2(state.mat.roughness, random2);
        H = state.tangent * H.x + state.bitangent * H.y + N * H.z;
        float3 R = reflect(-V, H);
        float F = Fresnel_Dielectric(abs(dot(R, H)), state.eta);

        if (rnd(seed) < F) {  // reflection
            L = normalize(R);
            bxdf = EvalDielectricReflection(state, V, L, N, H, out_pdf);
        } else {  // transmission
            L = normalize(refract(-V, H, state.eta));
            bxdf = EvalDielectricRefraction(state, V, L, N, H, out_pdf);
        }
        bxdf *= trans_weight;
        out_pdf *= trans_weight;
    } else {  // sample brdf
        if (rnd(seed) < diffuse_ratio) {
            if (rnd(seed) < state.mat.subsurface) {  // subsurface
                float3 sample = UniformSampleHemisphere(random2);
                L = state.tangent * sample.x + state.bitangent * sample.y + N * sample.z;

                bxdf = EvalSubsurface(state, V, L, N, out_pdf);
                out_pdf *= state.mat.subsurface * diffuse_ratio;
                state.is_subsurface = true;
            } else {  // diffuse
                float4 sample = CosSampleHemisphere(random2);
                L = state.tangent * sample.x + state.bitangent * sample.y + N * sample.z;
                float3 H = normalize(L + V);
                bxdf = EvalDiffuse(state, Csheen, V, L, N, H, out_pdf);
                out_pdf *= (1.f - state.mat.subsurface) * diffuse_ratio;
            }
        } else {
            if (rnd(seed) < primary_spec_ratio) {  // specular
                float3 H = ImportanceSampleGTR2_aniso(state.mat.ax, state.mat.ay, random2);
                H = state.tangent * H.x + state.bitangent * H.y + N * H.z;
                L = normalize(reflect(-V, H));

                bxdf = EvalSpecular(state, Cspec0, V, L, N, H, out_pdf);
                out_pdf *= primary_spec_ratio * (1.f - diffuse_ratio);
            } else {  // clearcoat
                float3 H = ImportanceSampleGTR1(state.mat.roughness, random2);
                H = state.tangent * H.x + state.bitangent * H.y + N * H.z;
                L = normalize(reflect(-V, H));

                bxdf = EvalClearcoat(state, V, L, N, H, out_pdf);
                out_pdf *= (1.f - primary_spec_ratio) * (1.f - diffuse_ratio);
            }
        }
        bxdf *= (1.f - trans_weight);
        out_pdf *= (1.f - trans_weight);
    }

    out_bxdf = bxdf;
    out_dir = L;
}
