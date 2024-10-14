//
// Created by HUSTLX on 2024/10/12.
//

#ifndef HARDWAREPATHTRACER_TEXTURESHARED_H
#define HARDWAREPATHTRACER_TEXTURESHARED_H

#include "core/Core.h"
#include "stb_image.h"


namespace HWPT {
    enum class TextureFormat : uint8_t {
        None = 0x0,
        RGB,
        RGBA,
        Depth32,
        Depth32Stencil8,
        Depth24Stencil8
    };

    auto GetTextureFormat(int Channels) -> TextureFormat;

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
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_TEXTURESHARED_H
