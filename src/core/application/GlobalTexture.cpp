//
// Created by HUSTLX on 2025/2/12.
//

#include "GlobalTexture.h"


namespace Shadowy {

    void
    GlobalTexture::CreateGlobalTexture(TextureType Type, uint Width, uint Height, VkFormat Format,
                                       VkImageUsageFlags Usage, VkImageLayout InitialLayout) {
        uint Index = static_cast<uint>(Type);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_textures[Index][i] = new Texture2D(Width, Height, Format, Usage, InitialLayout);
        }
    }
}  // namespace Shadowy
