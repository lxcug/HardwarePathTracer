//
// Created by HUSTLX on 2024/10/12.
//

#ifndef HARDWAREPATHTRACER_TEXTURESHARED_H
#define HARDWAREPATHTRACER_TEXTURESHARED_H

#include "core/Core.h"
#include "stb_image.h"


namespace Shadowy {
    enum class TextureFormat : uint8_t {
        None = 0x0,
        RGB,
        SRGBA,
        RGBA_UNORM,
        RGBA_SNORM,
        RGBA16_SFLOAT,
        RGBA32_SFLOAT,
        Depth32,
        Depth32Stencil8,
        Depth24Stencil8
    };

    auto GetTextureFormat(int Channels, bool IsSRGB) -> TextureFormat;

    auto GetVKFormat(TextureFormat Format) -> VkFormat;

    auto IsDepthStencilTexture(TextureFormat Format) -> bool;

    auto GetVKSampleCount(uint SampleCount) -> VkSampleCountFlagBits;
  
    enum class TextureUsage : uint8_t {
        None = 0x0,
        ColorAttachment,
        DepthStencilAttachment,
        ColorAttachmentMSAA,
        DepthStencilAttachmentMSAA,
        RTV,
        SRV,
        UAV,
        DSV,  // TODO: Depth Stencil View
        CBV  // TODO: Const Buffer View
    };
}  // namespace Shadowy

#endif //HARDWAREPATHTRACER_TEXTURESHARED_H
