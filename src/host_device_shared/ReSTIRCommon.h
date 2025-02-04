//
// Created by HUSTLX on 2025/2/4.
//

#ifndef SHADOWY_RESTIRCOMMON_H
#define SHADOWY_RESTIRCOMMON_H

#include "BaseDefinitions.h"

BEGIN_SHADOWY_NAMESPACE

    struct ReSTIRSample {
        float3 vis_point, vis_point_normal;
        float3 sample_point, sample_point_normal;
        float3 sample_point_Lo;
    };

#if IS_COMPILING_SHADER
    float SourceDistribution(in ReSTIRSample sample) {
        return 1 / (2 * PI);
    }

    float TargetDistribution(in ReSTIRSample sample) {
        return Luminance(sample.sample_point_Lo);
    }
#endif

    struct Reservoir {
        ReSTIRSample y;
        float w_sum;
        int M;
        float W;

#if IS_COMPILING_SHADER
        void Init() {
            w_sum = 0.f;
            M = 0;
        }

        void Update(in ReSTIRSample sample, in float wi, inout uint seed) {
            w_sum += wi;
            M++;
            if (rnd(seed) < wi / w_sum) {
                y = sample;
            }
        }

        void Finalize() {
            W = 1.f / (TargetDistribution(y) * M) * w_sum;
        }

        void Merge(in Reservoir other, inout uint seed) {
            int Total = M + other.M;
            float reservoir_weight = TargetDistribution(other.y) * other.W * other.M;
            Update(other.y, reservoir_weight, seed);
            M = Total;
            Finalize();
        }
#endif
    };

END_SHADOWY_NAME_SPACE

#endif //SHADOWY_RESTIRCOMMON_H
