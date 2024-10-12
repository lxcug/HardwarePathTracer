//
// Created by HUSTLX on 2024/10/12.
//

#ifndef HARDWAREPATHTRACER_TEXTURE2D_H
#define HARDWAREPATHTRACER_TEXTURE2D_H

#include "TextureShared.h"
#include <filesystem>
#include "core/texture/Sampler.h"


namespace HWPT {
    // TODO
    class Texture2DDesc {
    public:
        uint m_width, m_height;
        TextureFormat m_format;
        TextureUsage m_usage;
    };

    class Texture2D {
    public:
        void CreateTexture(const std::filesystem::path& TexturePath);

        VkImageView CreateSRV();

        ~Texture2D();

    private:
        VkImage m_texture = VK_NULL_HANDLE;
        VkDeviceMemory m_textureMemory = VK_NULL_HANDLE;
        uint m_width = 0, m_height = 0;
        TextureFormat m_format = TextureFormat::None;
        void* m_mappedData = nullptr;
        bool IsSRVCreated = false;
        VkImageView m_textureView = VK_NULL_HANDLE;
    };
}

#endif //HARDWAREPATHTRACER_TEXTURE2D_H
