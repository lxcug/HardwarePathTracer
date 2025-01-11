//
// Created by HUSTLX on 2025/1/11.
//

#ifndef HARDWAREPATHTRACER_SWAPCHAIN_H
#define HARDWAREPATHTRACER_SWAPCHAIN_H

#include "Core.h"
#include "texture/Texture2D.h"


namespace HWPT {
    // TODO: Merge into swapchain
    struct LastFrameTextures {
        Texture2D LastFrameSceneColor;
    };

}  // namespace HWPT

#endif //HARDWAREPATHTRACER_SWAPCHAIN_H
