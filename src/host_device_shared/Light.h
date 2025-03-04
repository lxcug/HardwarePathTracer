//
// Created by HUSTLX on 2025/1/15.
//

#ifndef LIGHT_H
#define LIGHT_H

#include "BaseDefinitions.h"


BEGIN_SHADOWY_NAMESPACE

    enum class LightType
    {
        Directional = 0x0,
        Point,
        Spot,
        Rect,
        Sky,
        LightTypeMax = 0x7fffffff
    };

    struct Light
    {
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(LightType, Type, LightType::LightTypeMax);
        float3 Direction;

        float3 Position;
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, Radius, .1f);

        // Set Default Color to Red for Debug
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float3, Color, float3(1.f, 0.f, 0.f));
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, Intensity, 1.f);

        // For directional light, half angle is 2\degree, Range for point light
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, HalfSinAngleOrRange, 0.01);
        float InnerAngle;
        float OuterAngle;
        int Padding;
    };

END_SHADOWY_NAME_SPACE

#endif //LIGHT_H
