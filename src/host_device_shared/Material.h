//
// Created by HUSTLX on 2025/1/15.
//

#ifndef MATERIAL_H
#define MATERIAL_H

#include "BaseDefinitions.h"


BEGIN_HWPT_NAMESPACE

    struct Material
    {
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float3, Albedo, float3(.5f, .5f, .5f));
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, AlbedoTextureID, -1);

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float3, Emissive, float3(0.f, 0.f, 0.f));
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, EmissiveTextureID, -1);

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float3, Transmittance, float3(1.f, 1.f, 1.f));
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, Opacity, 1.f);

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, Roughness, 1.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, Metallic, 0.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, RoughnessTextureID, -1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, MetallicTextureID, -1);
    };

END_HWPT_NAME_SPACE

#endif //MATERIAL_H
