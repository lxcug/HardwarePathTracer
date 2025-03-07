//
// Created by HUSTLX on 2025/1/15.
//

#ifndef RENDEROPTIONS_H
#define RENDEROPTIONS_H

#include "BaseDefinitions.h"

BEGIN_SHADOWY_NAMESPACE
    struct PathTracingOptions
    {
#define NO_DEBUG 0
#define DEBUG_BASE_COLOR 1
#define DEBUG_NORMAL 2
#define DEBUG_EMISSION 3
#define DEBUG_ROUGHNESS 4
#define DEBUG_METALLIC 5
#define DEBUG_UV 6
#define DEBUG_INSTANCE_ID 7
#define DEBUG_OPACITY 8
        /*
         * 0 for path tracing
         * 1 for base color
         * 2 for normal
         * 3 for roughness
         * 4 for metallic
         */
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, DebugMode, NO_DEBUG);

#define MATERIAL_MODE_DISNEY 0
#define MATERIAL_MODE_PBR 1
        /*
         * 0 for disney
         * 1 for pbr
         */
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, MaterialMode, MATERIAL_MODE_DISNEY);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, FireFlyThreshold, 4.f);
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
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, DiffuseIntensity, 1.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, SpecularIntensity, 1.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(float, EmissionIntensity, 1.f);
        DECLARE_MEMBER_WITH_DEFAULT_VALUE(int, IsPickingObject, 0);
        float4 RayOrigin;  // .z > 0.f for ReTracePickingRay
        float4 RayDirection;
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
