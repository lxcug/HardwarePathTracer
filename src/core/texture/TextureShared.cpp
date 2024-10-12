//
// Created by HUSTLX on 2024/10/12.
//

#include "TextureShared.h"


namespace HWPT {
    TextureFormat GetTextureFormat(int Channels) {
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

    VkFormat GetVKFormat(TextureFormat Format) {
        return VK_FORMAT_R8G8B8A8_SRGB;  // TODO
        switch (Format) {
            case TextureFormat::RGB:
                return VK_FORMAT_R8G8B8_SRGB;
            case TextureFormat::RGBA:
                return VK_FORMAT_R8G8B8A8_SRGB;
            case TextureFormat::None: [[fallthrough]];
            default:
                Check(false);
                return VK_FORMAT_UNDEFINED;
        }
    }
}
