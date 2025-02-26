//
// Created by HUSTLX on 2024/10/12.
//

#ifndef HARDWAREPATHTRACER_TEXTURE2D_H
#define HARDWAREPATHTRACER_TEXTURE2D_H

#include "TextureShared.h"
#include <filesystem>
#include "core/texture/Sampler.h"
#include "assimp/texture.h"


// TODO: Reconstruct this...
namespace Shadowy {
    // TODO
//    class Texture2DDesc {
//    public:
//        uint m_width, m_height;
//        TextureFormat m_format;
//        TextureUsage m_usage;
//    };

    class Texture2D {
    public:
        explicit Texture2D(const std::filesystem::path &TexturePath,
                           bool GenerateMips = false, bool SRGB = false);

        explicit Texture2D(const aiTexture* AITexture,
                           bool GenerateMips = false, bool SRGB = false);

        Texture2D(uint Width, uint Height, VkFormat Format, VkImageUsageFlags Usage,
            VkImageLayout InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED, uint MSAASamples = 1);

        ~Texture2D();

        void CreateTexture(const std::filesystem::path &TexturePath);

        void CreateTexture(stbi_uc* Data);

        auto CreateSRV() -> VkImageView;

        auto GetHandle() -> VkImage & {
            return m_texture;
        }

    private:
        static auto CalculateNumMips(uint Width, uint Height) -> uint;

        VkImage m_texture = VK_NULL_HANDLE;
        VkDeviceMemory m_textureMemory = VK_NULL_HANDLE;
        int m_width = 0, m_height = 0, m_channels;
        bool m_useNativeVkFormat = false;
        VkFormat m_vkFormat;
        bool IsSRVCreated = false;
        VkImageView m_textureView = VK_NULL_HANDLE;
        uint m_numMips = 1;
        bool m_generateMips = false;
        uint m_msaaSamples = 1;
        bool m_isSRGB = false;
    };
}  // namespace Shadowy

#endif //HARDWAREPATHTRACER_TEXTURE2D_H
