//
// Created by HUSTLX on 2025/2/4.
//

#ifndef SHADOWY_RESTIRCOMMON_H
#define SHADOWY_RESTIRCOMMON_H

#include "BaseDefinitions.h"


BEGIN_SHADOWY_NAMESPACE

    struct ReSTIRDISample {
        float3 vis_point;
        uint padding0;
        float3 direct_luminance_dir;
        uint padding1;
        float3 radiance;
        uint padding2;
    };

    struct ReSTIRGISample {
        float3 vis_point;
        bool is_valid_sample;
        float3 vis_point_normal;
        bool second_bounce_hit_sky;
        float3 sample_point;
        float vis_point_roughness;
        float3 sample_point_normal;
        float cos_theta;
        float3 sample_point_Lo;
        float second_ray_pdf;
        float3 second_dir;
        float vis_point_metallic;
        float3 vis_point_albedo;
        uint seed;
        float3 primary_dir;
        uint padding3;

        void Init() {
            seed = 0;
            is_valid_sample = false;
            second_bounce_hit_sky = false;
            sample_point_Lo = float3(0.f, 0.f, 0.f);
            vis_point = float3(0.f, 0.f, 0.f);
            vis_point_normal = float3(0.f, 0.f, 0.f);
            sample_point = float3(0.f, 0.f, 0.f);
            sample_point_normal = float3(0.f, 0.f, 0.f);
            vis_point_albedo = float3(0.f, 0.f, 0.f);
            second_ray_pdf = 0.f;
            second_dir = float3(0.f, 0.f, 0.f);
            primary_dir = float3(0.f, 0.f, 0.f);
        }
    };

#if IS_COMPILING_SHADER

    float SourceDistribution(in ReSTIRGISample sample) {
//        float NoL = dot(sample.second_dir, sample.vis_point_normal);
//        return NoL / PI;
        return sample.second_ray_pdf;
    }

    float TargetDistribution(in ReSTIRGISample sample, in float3 V);
#endif

    struct ReSTIRGIReservoir {
        float w;
        int M;
        float W;
        uint Flag;
        ReSTIRGISample y;

#if IS_COMPILING_SHADER
        void Init() {
            w = 0.f;
            M = 0;
            W = 0.f;
            y.Init();
        }

        void Update(in ReSTIRGISample sample, in float wi, inout uint seed) {
            w += wi;
            M++;
            if (rnd(sample.seed) < wi / w) {
                y = sample;
            }
        }

        float3 RISEstimator() {
            return y.sample_point_Lo * W;
        }

        void UpdateW() {
            W = w / (TargetDistribution(y, -y.primary_dir) * M);
        }

        void Merge(in ReSTIRGIReservoir other, in float p, inout uint seed) {
            int Total = M + other.M;
            float ReSTIRGIReservoir_weight = p * other.W * other.M;
            Update(other.y, ReSTIRGIReservoir_weight, seed);
            M = Total;
            W = w / (TargetDistribution(y, -y.primary_dir) * M);
        }
#endif
    };

END_SHADOWY_NAME_SPACE

#endif //SHADOWY_RESTIRCOMMON_H
