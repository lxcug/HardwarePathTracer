//
// Created by HUSTLX on 2025/1/15.
//

#ifndef RENDEROPTIONS_H
#define RENDEROPTIONS_H

#include "BaseDefinitions.h"

BEGIN_HWPT_NAMESPACE
    struct PathTracingOptions
    {
        // TODO: Pack Bool into Int/UInt
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EnableAccumulation, 1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EnableAO, 0);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, NumAORays, 4);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, AORayLength, 1.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, NumLights, 0);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, Bounce, 4);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, RayMinBias, 1e-3f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, MaxTraceDistance, 1e4f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EnableSkyLight, 1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EnableEmissive, 0);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, MISMode, 2);  // 0 for NEE 1 for sample material 2 for MIS
    };

    struct ToneMappingOptions {
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EnableGammaCorrection, 1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EnableToneMapping, 1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, AdaptedLuminance, 1.f);
    };

END_HWPT_NAME_SPACE

#endif //RENDEROPTIONS_H
