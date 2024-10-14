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

    auto GetVKSampleCount(uint SampleCount) -> VkSampleCountFlagBits {
        switch (SampleCount) {
            case 1:
                return VK_SAMPLE_COUNT_1_BIT;
            case 2:
                return VK_SAMPLE_COUNT_2_BIT;
            case 4:
                return VK_SAMPLE_COUNT_4_BIT;
            case 8:
                return VK_SAMPLE_COUNT_8_BIT;
            case 16:
                return VK_SAMPLE_COUNT_16_BIT;
            case 32:
                return VK_SAMPLE_COUNT_32_BIT;
            case 64:
                return VK_SAMPLE_COUNT_64_BIT;
            default:
                Check(false);
                return VK_SAMPLE_COUNT_1_BIT;
        }
    }
}  // namespace HWPT
