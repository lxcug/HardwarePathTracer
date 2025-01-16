//
// Created by HUSTLX on 2025/1/15.
//

#ifndef RENDEROPTIONS_H
#define RENDEROPTIONS_H

#include "BaseDefinitions.h"

BEGIN_HWPT_NAMESPACE
    struct RenderOptions
    {
        // TODO: Pack Bool into Int/UInt
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EnableAccumulation, 1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EnableAO, 0);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, ShouldReAccumulate, 1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, NumAORays, 4);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, AORayLength, 1.f);
        uint NumLights;
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, Bounce, 4);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, RayMinBias, 1e-3f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, MaxTraceDistance, 1e4f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, AccumulateSkyLight, 1);
    };

#if IS_COMPILING_SHADER
[[vk::push_constant]] RenderOptions render_options;
#endif

END_HWPT_NAME_SPACE

#endif //RENDEROPTIONS_H
