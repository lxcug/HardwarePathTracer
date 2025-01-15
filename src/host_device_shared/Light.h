//
// Created by HUSTLX on 2025/1/15.
//

#ifndef LIGHT_H
#define LIGHT_H

#include "BaseDefinitions.h"


BEGIN_HWPT_NAMESPACE

    enum class LightType
    {
        Directional = 0x0,
        Point,
        Spot,
        Rect,
        LightTypeMax = 0x7fffffff
    };

    struct Light
    {
        float3 Direction;
        LightType Type;

        float3 Position;
        float Radius;

        float3 Color;
        float Intensity;

        float AngleSizeOrInvRange;
        float InnerAngle;
        float OuterAngle;
        int Padding;
    };

END_HWPT_NAME_SPACE

#endif //LIGHT_H
