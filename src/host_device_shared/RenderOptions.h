//
// Created by HUSTLX on 2025/1/15.
//

#ifndef RENDEROPTIONS_H
#define RENDEROPTIONS_H

#include "BaseDefinitions.h"

BEGIN_SHADOWY_NAMESPACE
    struct PathTracingOptions
    {
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EnableAO, 0);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, NumAORays, 4);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, AORayLength, 1.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, NumLights, 0);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, Bounce, 6);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, RayMinBias, 1e-3f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, MaxTraceDistance, 1e5f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EnableSkyLight, 1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EnableEmissive, 0);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, MISMode, 2);  // 0 for NEE 1 for sample material 2 for MIS
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, DeferredTraceMaterial, 1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EnableReSTIRGI, 0);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, ShowIndirectOnly, 0);
    };

    struct ToneMappingOptions {
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EnableGammaCorrection, 1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EnableToneMapping, 1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, AdaptedLuminance, 1.f);
    };

    struct TemporalReuseOptions {
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, MaxAccumulatedFrames, 1);
    };

    struct SpatialReuseOptions {
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, ReuseRadius, 1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EnableSpatialReuse, 0);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, MaxAccumulatedFrames, 1);
    };

    struct AccumulationOptions {
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EnableAccumulation, 1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, MaxAccumulatedFrames, 0);
    };

END_SHADOWY_NAME_SPACE

#endif //RENDEROPTIONS_H
