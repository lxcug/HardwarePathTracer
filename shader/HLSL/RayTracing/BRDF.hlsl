#pragma once

float3 diffuse_lambert(float3 albedo) {
    return albedo * (1 / PI);
}