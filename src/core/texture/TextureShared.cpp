//
// Created by HUSTLX on 2024/10/12.
//

#include "TextureShared.h"


namespace HWPT {
    auto GetTextureFormat(int Channels) -> TextureFormat {
        switch (Channels) {
            case 3:
                return TextureFormat::RGB;
            case 4:
                return TextureFormat::RGBA;
            default:
                Check(false);
                return TextureFormat::None;
        }
    }

    auto GetVKFormat(TextureFormat Format) -> VkFormat {
        switch (Format) {
            case TextureFormat::RGB:
                [[fallthrough]];
            case TextureFormat::RGBA:
                return VK_FORMAT_R8G8B8A8_SRGB;
            case TextureFormat::Depth32:
                return VK_FORMAT_D32_SFLOAT;
            case TextureFormat::Depth32Stencil8:
                return VK_FORMAT_D32_SFLOAT_S8_UINT;
            case TextureFormat::Depth24Stencil8:
                return VK_FORMAT_D24_UNORM_S8_UINT;
            case TextureFormat::None:
                [[fallthrough]];
            default:
                Check(false);
                return VK_FORMAT_UNDEFINED;
        }
    }

    auto IsDepthStencilTexture(TextureFormat Format) -> bool {
        return Format == TextureFormat::Depth32 || Format == TextureFormat::Depth32Stencil8 ||
               Format == TextureFormat::Depth24Stencil8;
    }
}  // namespace HWPT
