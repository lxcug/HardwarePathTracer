//
// Created by HUSTLX on 2025/1/15.
//

#ifndef SHADOWY_MATERIAL_H
#define SHADOWY_MATERIAL_H

#include "BaseDefinitions.h"
#include "Light.h"


BEGIN_SHADOWY_NAMESPACE

#define ALPHA_MODE_OPAQUE 0
#define ALPHA_MODE_MASK 1
#define ALPHA_MODE_TRANSPARENT 2

    /*
     * Offers Addresses for query Vertex/Index/Material Buffer
     */
    struct InstanceData
    {
        uint64_t VertexBufferAddress;
        uint64_t IndexBufferAddress;
        uint64_t MaterialBufferAddress;
        uint MaterialIndex;
    };

    /*
     * Should align with 4B cause this structure is stored in StructuredBuffer.
     * Stores material read from raw 3d asset(No texture sampling executed).
     */
    struct InputMaterial {
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float3, base_color, float3(1.f, 1.f, 1.f));
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, base_color_tex_id, -1);

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float3, emission, float3(0.f, 0.f, 0.f));
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, emission_tex_id, -1);

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, normal_tex_id, -1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, specular_tex_id, -1);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, roughness, .5f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, metallic, 0.f);

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, metallic_factor, 1.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, roughness_factor, 1.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, transmission, 0.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, opacity, 1.f);

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, ior, 1.5f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, two_sided, 0);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(uint, alpha_mode, ALPHA_MODE_OPAQUE);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, alpha_cutoff, 0.f);

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, specular_tint, 0.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, specular, .5f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, clearcoat, 0.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, clearcoat_roughness, 0.f);

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, sheen, 1.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float3, sheen_tint, float3(1.f, 1.f, 1.f));

        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, subsurface, 0.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, anisotropy, 0.f);
        float2 padding;
    };
END_SHADOWY_NAME_SPACE

#endif //SHADOWY_MATERIAL_H
