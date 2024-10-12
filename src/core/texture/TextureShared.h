//
// Created by HUSTLX on 2024/10/12.
//

#ifndef HARDWAREPATHTRACER_TEXTURESHARED_H
#define HARDWAREPATHTRACER_TEXTURESHARED_H

#include "core/Core.h"
#include "stb_image.h"


namespace HWPT {
    enum class TextureFormat {
        None = 0x0,
        RGB,
        RGBA
    };

    TextureFormat GetTextureFormat(int Channels);

    VkFormat GetVKFormat(TextureFormat Format);

    enum class TextureUsage {
        None = 0x0,
        RTV,
        SRV,
        UAV,
        DSV,  // TODO: Depth Stencil View
        CBV  // TODO: Const Buffer View
    };
}

#endif //HARDWAREPATHTRACER_TEXTURESHARED_H
