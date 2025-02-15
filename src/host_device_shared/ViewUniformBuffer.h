//
// Created by HUSTLX on 2025/1/15.
//

#ifndef VIEWUNIFORMBUFFER_H
#define VIEWUNIFORMBUFFER_H

#include "BaseDefinitions.h"


BEGIN_SHADOWY_NAMESPACE

struct ViewUniformBuffer
{
    float4x4 ViewTrans;
    float4x4 ProjTrans;

    float4x4 InvView;
    float4x4 InvProj;

    float3 CameraPos;
    uint AccumulatedFrameNum;

    uint FrameNum;
    float DeltaTime;

    int Width;
    int Height;
};

END_SHADOWY_NAME_SPACE

#endif //VIEWUNIFORMBUFFER_H
