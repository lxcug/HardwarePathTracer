//
// Created by HUST4e34LX on 2024/10/12.
//

#include "Texture2D.h"
#include "core/RHI.h"

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"


namespace Shadowy {
    // TextureUsage is ShaderResourceView when giving a TexturePath
    Texture2D::Texture2D(const std::filesystem::path &TexturePath, uint MSAASamples,
                         bool GenerateMips, bool SRGB)
            : m_msaaSamples(MSAASamples), m_generateMips(GenerateMips),
              m_textureUsage(TextureUsage::SRV), m_isSRGB(SRGB) {
        CreateTexture(TexturePath);
    }

    Texture2D::Texture2D(uint Width, uint Height, VkFormat Format, VkImageUsageFlags Usage,
                         VkImageLayout InitialLayout)
    : m_width(Width), m_height(Height), m_useNativeVkFormat(true), m_vkFormat(Format) {
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

        RHI::TextureTransitionInput SrcInput{}, DstInput{};
        SrcInput.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
        SrcInput.AccessMask = 0;
        SrcInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
        DstInput.Layout = InitialLayout;
        DstInput.AccessMask = 0;
        DstInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
        RHI::TransitionTextureLayout(m_texture, 1, SrcInput, DstInput);
    }

    Texture2D::Texture2D(uint Width, uint Height, TextureFormat Format, TextureUsage Usage,
                         uint MSAASamples, bool GenerateMips)
            : m_width(Width), m_height(Height), m_format(Format),
              m_textureUsage(Usage), m_msaaSamples(MSAASamples), m_generateMips(GenerateMips) {
        switch (m_textureUsage) {
            case TextureUsage::ColorAttachment:
                RHI::CreateTexture2D(Width, Height, m_numMips, GetVKSampleCount(m_msaaSamples),
                                     GetVKFormat(m_format),
                                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                     VK_IMAGE_TILING_OPTIMAL, m_texture, m_textureMemory);
                break;
            case TextureUsage::DepthStencilAttachment:
                RHI::CreateTexture2D(Width, Height, m_numMips, GetVKSampleCount(m_msaaSamples),
                                     GetVKFormat(m_format),
                                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                     VK_IMAGE_TILING_OPTIMAL, m_texture, m_textureMemory);
                break;
            case TextureUsage::ColorAttachmentMSAA:
                RHI::CreateTexture2D(Width, Height, m_numMips, GetVKSampleCount(m_msaaSamples),
                                     GetVKFormat(m_format),
                                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                     VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
                                     VK_IMAGE_TILING_OPTIMAL, m_texture, m_textureMemory);
                break;
            case TextureUsage::DepthStencilAttachmentMSAA:
                RHI::CreateTexture2D(Width, Height, m_numMips, GetVKSampleCount(m_msaaSamples),
                                     GetVKFormat(m_format),
                                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                     VK_IMAGE_TILING_OPTIMAL, m_texture, m_textureMemory);
                break;
            case TextureUsage::SRV:
                RHI::CreateTexture2D(Width, Height, m_numMips, GetVKSampleCount(m_msaaSamples),
                                     GetVKFormat(m_format),
                                     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                     VK_IMAGE_TILING_OPTIMAL, m_texture, m_textureMemory);
                break;
            case TextureUsage::UAV:
                RHI::CreateTexture2D(Width, Height, m_numMips, VK_SAMPLE_COUNT_1_BIT,
                                     GetVKFormat(m_format),
                                     VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                     VK_IMAGE_TILING_LINEAR, m_texture, m_textureMemory);
                break;
            default:
                throw std::runtime_error("Unsupported TextureUsage");
        }
    }

    void Texture2D::CreateTexture(const std::filesystem::path &TexturePath) {
        Check(m_textureUsage != TextureUsage::None);

        int Channels;
        stbi_set_flip_vertically_on_load(false);
        stbi_uc *Pixels = stbi_load(TexturePath.string().c_str(),
                                    reinterpret_cast<int *>(&m_width),
                                    reinterpret_cast<int *>(&m_height),
                                    &Channels, STBI_rgb_alpha);
        Check(Pixels);
        m_format = GetTextureFormat(Channels, m_isSRGB);

        if (m_generateMips) {
            m_numMips = CalculateNumMips(m_width, m_height);
        }

        VkDeviceSize MemorySize = m_width * m_height * 4;
        auto [StagingBuffer, StagingBufferMemory] = RHI::CreateStagingBuffer(MemorySize);

        void *MappedData = nullptr;
        vkMapMemory(GetVKDevice(), StagingBufferMemory, 0, MemorySize, 0, &MappedData);
        memcpy(MappedData, Pixels, MemorySize);
        vkUnmapMemory(GetVKDevice(), StagingBufferMemory);
        stbi_image_free(Pixels);

        RHI::CreateTexture2D(m_width, m_height, m_numMips, GetVKSampleCount(m_msaaSamples),
                             GetVKFormat(m_format),
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
        CreateInfo.format = m_useNativeVkFormat ? m_vkFormat : GetVKFormat(m_format);
        CreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        VkImageAspectFlags AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        if (IsDepthStencilTexture(m_format)) {
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
