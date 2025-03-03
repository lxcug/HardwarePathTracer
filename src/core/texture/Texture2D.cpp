//
// Created by HUST4e34LX on 2024/10/12.
//

#include "Texture2D.h"
#include "core/RHI.h"

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"
#include "dds.hpp"
#include "core/Utils.h"


namespace Shadowy {
    // TextureUsage is ShaderResourceView when giving a TexturePath
    Texture2D::Texture2D(const std::filesystem::path &TexturePath,
                         bool GenerateMips, bool SRGB)
            : m_generateMips(GenerateMips), m_isSRGB(SRGB) {
        CreateTexture(TexturePath);
    }

    Texture2D::Texture2D(const aiTexture* AITexture, bool GenerateMips, bool SRGB)
        :  m_generateMips(GenerateMips), m_isSRGB(SRGB)
    {
        if (AITexture->mHeight == 0)  // Texture is compressed
        {
            std::string TexturePath = AITexture->mFilename.C_Str();
            size_t DotPos = TexturePath.find_last_of('.');
            std::string Extension =TexturePath.substr(DotPos);

            unsigned char* Data = nullptr;
            if (Extension == ".dds")
            {
                dds::Image Image;
                auto Result = dds::readFile(TexturePath, &Image);
                m_width = Image.width;
                m_height = Image.height;
                m_channels = Image.depth;
                Data = reinterpret_cast<unsigned char*>(Image.data.data());
                CreateTexture(Data, Image.data.size());
            }
            else
            {
                Data =
                    stbi_load_from_memory(reinterpret_cast<const unsigned char*>(AITexture->pcData),
                        AITexture->mWidth,
                        &m_width,
                        &m_height,
                        &m_channels,
                        STBI_rgb_alpha);
                CreateTexture(Data);
            }
        }
        else
        {
            m_channels = 4;
            CreateTexture(reinterpret_cast<stbi_uc*>(AITexture->pcData));
        }
    }

    Texture2D::Texture2D(uint Width, uint Height, VkFormat Format, VkImageUsageFlags Usage,
                         VkImageLayout InitialLayout, uint MSAASamples)
    : m_width(Width), m_height(Height), m_useNativeVkFormat(true), m_vkFormat(Format),
    m_msaaSamples(MSAASamples) {
        VkImageCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        CreateInfo.imageType = VK_IMAGE_TYPE_2D;
        CreateInfo.extent.width = Width;
        CreateInfo.extent.height = Height;
        CreateInfo.extent.depth = 1;
        CreateInfo.mipLevels = 1;
        CreateInfo.arrayLayers = 1;
        CreateInfo.format = Format;
        CreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        CreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        CreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        CreateInfo.usage = Usage;
        CreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        CreateInfo.samples = GetVKSampleCount(m_msaaSamples);

        VK_CHECK(vkCreateImage(GetVKDevice(), &CreateInfo, nullptr, &m_texture));

        VkMemoryRequirements MemRequirements;
        vkGetImageMemoryRequirements(GetVKDevice(), m_texture, &MemRequirements);
        VkMemoryAllocateInfo AllocateInfo{};
        AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        AllocateInfo.allocationSize = MemRequirements.size;
        AllocateInfo.memoryTypeIndex = RHI::FindMemoryType(MemRequirements.memoryTypeBits,
                                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(GetVKDevice(), &AllocateInfo, nullptr, &m_textureMemory);
        vkBindImageMemory(GetVKDevice(), m_texture, m_textureMemory, 0);

        if (InitialLayout != VK_IMAGE_LAYOUT_UNDEFINED)
        {
            RHI::TextureTransitionInput SrcInput{}, DstInput{};
            SrcInput.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
            SrcInput.AccessMask = 0;
            SrcInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
            DstInput.Layout = InitialLayout;
            DstInput.AccessMask = 0;
            DstInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
            RHI::TransitionTextureLayout(m_texture, 1, SrcInput, DstInput);
        }
    }

    void Texture2D::CreateTexture(const std::filesystem::path &TexturePath) {
        if (TexturePath.extension().string() == ".dds")
        {
            dds::Image Image;
            auto Result = dds::readFile(TexturePath.string(), &Image);
            m_width = Image.width;
            m_height = Image.height;
            m_channels = 4;

            m_vkFormat = dds::getVulkanFormat(Image.format, Image.supportsAlpha);
            // Will automatically fill VkImageCreateInfo::format with a separate call to dds::getVulkanFormat.
            VkImageCreateInfo ImageCreateInfo = dds::getVulkanImageCreateInfo(&Image);
            ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            ImageCreateInfo.mipLevels = 1;
            ImageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            VK_CHECK(vkCreateImage(GetVKDevice(), &ImageCreateInfo, nullptr, &m_texture));

            VkMemoryRequirements MemRequirements;
            vkGetImageMemoryRequirements(GetVKDevice(), m_texture, &MemRequirements);
            VkMemoryAllocateInfo AllocateInfo{};
            AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            AllocateInfo.allocationSize = MemRequirements.size;
            AllocateInfo.memoryTypeIndex = RHI::FindMemoryType(MemRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            auto Res = vkAllocateMemory(GetVKDevice(), &AllocateInfo, nullptr, &m_textureMemory);
            vkBindImageMemory(GetVKDevice(), m_texture, m_textureMemory, 0);

            // NOTE: Memory Size is not width * height * channel since texture is compressed
            auto [StagingBuffer, StagingBufferMemory] = RHI::CreateStagingBuffer(MemRequirements.size);

            void *MappedData = nullptr;
            vkMapMemory(GetVKDevice(), StagingBufferMemory, 0, MemRequirements.size, 0, &MappedData);
            memcpy(MappedData, Image.mipmaps[0].data(), MemRequirements.size);
            vkUnmapMemory(GetVKDevice(), StagingBufferMemory);

            RHI::TransitionTextureLayout(m_texture, m_numMips,
                                         VK_IMAGE_LAYOUT_UNDEFINED,
                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            RHI::CopyBufferToTexture(m_texture, StagingBuffer, m_width, m_height);

            vkDestroyBuffer(GetVKDevice(), StagingBuffer, nullptr);
            vkFreeMemory(GetVKDevice(), StagingBufferMemory, nullptr);

            RHI::TransitionTextureLayout(m_texture, m_numMips,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        else if (TexturePath.extension().string() == ".hdr" || TexturePath.extension().string() == ".exr")
        {
            stbi_set_flip_vertically_on_load(false);
            float* Pixels = stbi_loadf(TexturePath.string().c_str(),
                                        &m_width,
                                        &m_height,
                                        &m_channels, STBI_rgb_alpha);
            CreateTexture(Pixels);
        }
        else
        {
            stbi_set_flip_vertically_on_load(false);
            stbi_uc* Pixels = stbi_load(TexturePath.string().c_str(),
                                        &m_width,
                                        &m_height,
                                        &m_channels, STBI_rgb_alpha);
            CreateTexture(Pixels);
        }
    }

    void Texture2D::CreateTexture(stbi_uc* Data, int DataSize)
    {
        if (m_channels == 3 || m_channels == 4)
        {
            if (m_isSRGB)
            {
                m_vkFormat = VK_FORMAT_R8G8B8A8_SRGB;
            }
            else
            {
                m_vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
            }
        }
        // Roughness/Metallic Map as RGBA for Bindless
        if (m_channels == 1)
        {
            m_vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
        }

        Check(Data);

        if (m_generateMips) {
            m_numMips = CalculateNumMips(m_width, m_height);
        }

        VkDeviceSize MemorySize = DataSize > 0 ? DataSize : m_width * m_height * 4;
        auto [StagingBuffer, StagingBufferMemory] = RHI::CreateStagingBuffer(MemorySize);

        void *MappedData = nullptr;
        vkMapMemory(GetVKDevice(), StagingBufferMemory, 0, MemorySize, 0, &MappedData);
        memcpy(MappedData, Data, MemorySize);
        vkUnmapMemory(GetVKDevice(), StagingBufferMemory);
        stbi_image_free(Data);

        RHI::CreateTexture2D(m_width, m_height, m_numMips, GetVKSampleCount(m_msaaSamples),
                             m_vkFormat,
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                             VK_IMAGE_TILING_OPTIMAL, m_texture, m_textureMemory);
        RHI::TransitionTextureLayout(m_texture, m_numMips,
                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        RHI::CopyBufferToTexture(m_texture, StagingBuffer, m_width, m_height);

        if (m_generateMips) {
            RHI::GenerateMips(m_texture, m_width, m_height, m_numMips);
        } else {
            RHI::TransitionTextureLayout(m_texture, m_numMips,
                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        vkDestroyBuffer(GetVKDevice(), StagingBuffer, nullptr);
        vkFreeMemory(GetVKDevice(), StagingBufferMemory, nullptr);
    }

    void Texture2D::CreateTexture(float* Data)
    {
        m_vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
        Check(Data);

        if (m_generateMips) {
            m_numMips = CalculateNumMips(m_width, m_height);
        }

        VkDeviceSize MemorySize = m_width * m_height * 16;
        auto [StagingBuffer, StagingBufferMemory] = RHI::CreateStagingBuffer(MemorySize);

        void *MappedData = nullptr;
        vkMapMemory(GetVKDevice(), StagingBufferMemory, 0, MemorySize, 0, &MappedData);
        memcpy(MappedData, Data, MemorySize);
        vkUnmapMemory(GetVKDevice(), StagingBufferMemory);
        stbi_image_free(Data);

        RHI::CreateTexture2D(m_width, m_height, m_numMips, GetVKSampleCount(m_msaaSamples),
                             m_vkFormat,
                             VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                             VK_IMAGE_TILING_OPTIMAL, m_texture, m_textureMemory);
        RHI::TransitionTextureLayout(m_texture, m_numMips,
                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        RHI::CopyBufferToTexture(m_texture, StagingBuffer, m_width, m_height);

        if (m_generateMips) {
            RHI::GenerateMips(m_texture, m_width, m_height, m_numMips);
        } else {
            RHI::TransitionTextureLayout(m_texture, m_numMips,
                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        vkDestroyBuffer(GetVKDevice(), StagingBuffer, nullptr);
        vkFreeMemory(GetVKDevice(), StagingBufferMemory, nullptr);
    }

    Texture2D::~Texture2D() {
        if (IsSRVCreated) {
            vkDestroyImageView(GetVKDevice(), m_textureView, nullptr);
        }
        vkDestroyImage(GetVKDevice(), m_texture, nullptr);
        vkFreeMemory(GetVKDevice(), m_textureMemory, nullptr);
    }

    auto Texture2D::CreateSRV() -> VkImageView {
        if (IsSRVCreated) {
            return m_textureView;
        }

        VkImageViewCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        CreateInfo.image = m_texture;
        CreateInfo.format = m_vkFormat;
        CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

        VkImageAspectFlags AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        if (m_vkFormat == VK_FORMAT_D32_SFLOAT) {
            AspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        CreateInfo.subresourceRange.aspectMask = AspectFlags;
        CreateInfo.subresourceRange.baseMipLevel = 0;
        CreateInfo.subresourceRange.levelCount = m_numMips;
        CreateInfo.subresourceRange.baseArrayLayer = 0;
        CreateInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(GetVKDevice(), &CreateInfo, nullptr, &m_textureView));
        IsSRVCreated = true;
        return m_textureView;
    }

    auto Texture2D::CalculateNumMips(uint Width, uint Height) -> uint {
        uint MaxResolution = std::max(Width, Height);
        return static_cast<uint>(std::floor(std::log2(MaxResolution))) + 1;
    }
} // namespace Shadowy
