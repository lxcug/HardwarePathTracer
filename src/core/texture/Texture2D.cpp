//
// Created by HUST4e34LX on 2024/10/12.
//

#include "Texture2D.h"
#include "core/RHI.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


namespace HWPT {

    void Texture2D::CreateTexture(const std::filesystem::path &TexturePath) {
        int Channels;
        stbi_set_flip_vertically_on_load(false);
        stbi_uc* Pixels = stbi_load(TexturePath.string().c_str(),
                                    reinterpret_cast<int*>(&m_width),
                                    reinterpret_cast<int*>(&m_height),
                                    &Channels, STBI_rgb_alpha);
        Check(Pixels);
        m_format = GetTextureFormat(Channels);

        VkDeviceSize MemorySize = m_width * m_height * 4;  // TODO
        auto [StagingBuffer, StagingBufferMemory] = RHI::CreateStagingBuffer(MemorySize);

        void* MappedData = nullptr;
        vkMapMemory(GetVKDevice(), StagingBufferMemory, 0, MemorySize, 0, &MappedData);
        memcpy(MappedData, Pixels, MemorySize);
        vkUnmapMemory(GetVKDevice(), StagingBufferMemory);
        stbi_image_free(Pixels);

        RHI::CreateTexture2D(m_width, m_height, GetVKFormat(m_format),
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                             VK_IMAGE_TILING_OPTIMAL, m_texture, m_textureMemory);
        RHI::TransitionTextureLayout(m_texture, GetVKFormat(m_format),
                                     VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        RHI::CopyBufferToTexture(m_texture, StagingBuffer, m_width, m_height);
        RHI::TransitionTextureLayout(m_texture, GetVKFormat(m_format),
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(GetVKDevice(), StagingBuffer, nullptr);
        vkFreeMemory(GetVKDevice(), StagingBufferMemory, nullptr);
    }

    Texture2D::~Texture2D() {
        vkDestroyImage(GetVKDevice(), m_texture, nullptr);
        vkFreeMemory(GetVKDevice(), m_textureMemory, nullptr);
        vkDestroyImageView(GetVKDevice(), m_textureView, nullptr);
    }

    auto Texture2D::CreateSRV() -> VkImageView {
        if (IsSRVCreated) {
            return m_textureView;
        }

        VkImageViewCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        CreateInfo.image = m_texture;
        CreateInfo.format = GetVKFormat(m_format);
        CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        CreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        CreateInfo.subresourceRange.baseMipLevel = 0;
        CreateInfo.subresourceRange.levelCount = 1;
        CreateInfo.subresourceRange.baseArrayLayer = 0;
        CreateInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(GetVKDevice(), &CreateInfo, nullptr, &m_textureView));
        IsSRVCreated = true;
        return m_textureView;
    }

}  // namespace HWPT
