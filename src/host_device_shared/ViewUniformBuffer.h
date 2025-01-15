//
// Created by HUSTLX on 2025/1/15.
//

#ifndef VIEWUNIFORMBUFFER_H
#define VIEWUNIFORMBUFFER_H

#include "BaseDefinitions.h"


BEGIN_HWPT_NAMESPACE

#if IS_COMPILING_CPP
    struct ViewUniformBuffer
#else
cbuffer ViewUniformBuffer : register(b0)
#endif
    {
        float4x4 ModelTrans;
        float4x4 ViewTrans;
        float4x4 ProjTrans;

        float3 DebugColor;
        float DeltaTime;

        float3 CameraPos;
        uint FrameNum;

        float4x4 InvView;
        float4x4 InvProj;

        bool ShouldReAccumulate;
    };

END_HWPT_NAME_SPACE

#endif //VIEWUNIFORMBUFFER_H
