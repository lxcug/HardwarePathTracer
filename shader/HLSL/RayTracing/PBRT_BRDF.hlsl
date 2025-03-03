#pragma once

#include "Utils.hlsl"


float Fr_Dielectric(float cos_theta_i, float eta) {
    float sin_2_theta_i = 1.f - pow(cos_theta_i, 2);
    float cos_theta_t = sqrt(1 - sin_2_theta_i);

    float r_parl = (eta * cos_theta_i - cos_theta_t) / (eta * cos_theta_i + cos_theta_t);
    float r_perp = (cos_theta_i - eta * cos_theta_t) /
                   (cos_theta_i + eta * cos_theta_t);
    return (pow(r_parl, 2) + pow(r_perp, 2)) / 2.f;
}
