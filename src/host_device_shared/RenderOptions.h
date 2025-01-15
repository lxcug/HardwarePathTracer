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
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EnableAO, 1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, ShouldReAccumulate, 1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, NumAORays, 4);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, AORayLength, 5.f);

        uint NumLights;
    };

#if IS_COMPILING_SHADER
[[vk::push_constant]] RenderOptions render_options;
#endif

END_HWPT_NAME_SPACE

#endif //RENDEROPTIONS_H
