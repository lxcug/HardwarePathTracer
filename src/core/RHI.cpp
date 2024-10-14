//
// Created by HUSTLX on 2024/10/7.
//

#include "RHI.h"
#include "core/application/VulkanBackendApp.h"


namespace HWPT::RHI {

    auto FindMemoryType(uint TypeFilter, VkMemoryPropertyFlags Properties) -> uint {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(GetVKPhysicalDevice(), &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
            if (TypeFilter & (1 << i) &&
                (memoryProperties.memoryTypes[i].propertyFlags & Properties) == Properties) {
                return i;
            }
        }

        Check(false);
        return 0u;
    }

    void CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties,
                      VkBuffer &Buffer, VkDeviceMemory &BufferMemory) {
        VkBufferCreateInfo BufferInfo{};
        BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        BufferInfo.size = Size;
        BufferInfo.usage = Usage;
        BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkDevice GlobalDevice = GetVKDevice();
        VK_CHECK(vkCreateBuffer(GlobalDevice, &BufferInfo, nullptr, &Buffer));

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(GlobalDevice, Buffer, &memoryRequirements);

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits,
                                                      Properties);

        VK_CHECK(vkAllocateMemory(GlobalDevice, &allocateInfo, nullptr, &BufferMemory));

        vkBindBufferMemory(GlobalDevice, Buffer, BufferMemory, 0);
    }

    void CopyBuffer(VkBuffer Src, VkBuffer Dst, VkDeviceSize Size) {
        auto App = VulkanBackendApp::GetApplication();
        auto CommandBuffer = App->BeginIntermediateCommand();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = Size;
        vkCmdCopyBuffer(CommandBuffer, Src, Dst, 1, &copyRegion);

        App->EndIntermediateCommand(CommandBuffer);
    }

    auto CreateStagingBuffer(VkDeviceSize Size) -> std::tuple<VkBuffer, VkDeviceMemory> {
        VkBuffer StagingBuffer;
        VkDeviceMemory StagingBufferMemory;

        CreateBuffer(Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     StagingBuffer, StagingBufferMemory);

        return std::make_tuple(StagingBuffer, StagingBufferMemory);
    }

    void CreateTexture2D(uint Width, uint Height, uint NumMips, VkSampleCountFlagBits SampleCount,
                         VkFormat Format, VkImageUsageFlags Usage, VkImageTiling Tiling,
                         VkImage &Texture, VkDeviceMemory &TextureMemory) {
        VkImageCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        CreateInfo.imageType = VK_IMAGE_TYPE_2D;
        CreateInfo.extent.width = Width;
        CreateInfo.extent.height = Height;
        CreateInfo.extent.depth = 1;
        CreateInfo.mipLevels = NumMips;
        CreateInfo.arrayLayers = 1;
        CreateInfo.format = Format;
        CreateInfo.tiling = Tiling;
        CreateInfo.samples = SampleCount;
        CreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        if (NumMips > 1) {
            Usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        CreateInfo.usage = Usage;
        CreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK(vkCreateImage(GetVKDevice(), &CreateInfo, nullptr, &Texture));

        VkMemoryRequirements MemRequirements;
        vkGetImageMemoryRequirements(GetVKDevice(), Texture, &MemRequirements);
        VkMemoryAllocateInfo AllocateInfo{};
        AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        AllocateInfo.allocationSize = MemRequirements.size;
        AllocateInfo.memoryTypeIndex = FindMemoryType(MemRequirements.memoryTypeBits,
                                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(GetVKDevice(), &AllocateInfo, nullptr, &TextureMemory);
        vkBindImageMemory(GetVKDevice(), Texture, TextureMemory, 0);
    }

    void TransitionTextureLayout(VkImage Image, uint NumMips, VkImageLayout OldLayout,
                                 VkImageLayout NewLayout) {
        VkCommandBuffer CommandBuffer = VulkanBackendApp::GetApplication()->BeginIntermediateCommand();

        VkImageMemoryBarrier Barrier{};
        Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        Barrier.oldLayout = OldLayout;
        Barrier.newLayout = NewLayout;
        Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        Barrier.image = Image;
        Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        Barrier.subresourceRange.baseMipLevel = 0;
        Barrier.subresourceRange.levelCount = NumMips;
        Barrier.subresourceRange.baseArrayLayer = 0;
        Barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags SourceStage, DestinationStage;
        if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
            NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            Barrier.srcAccessMask = 0;
            Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            DestinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                   NewLayout == VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL) {
            Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            SourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            DestinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (OldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
                   NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            Barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            SourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            DestinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (OldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
                   NewLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
            Barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            Barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            SourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            DestinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        } else {
            throw std::runtime_error("Unsupported layout transition!");
        }

        vkCmdPipelineBarrier(CommandBuffer, SourceStage, DestinationStage,
                             0, 0, nullptr, 0, nullptr, 1, &Barrier);

        VulkanBackendApp::GetApplication()->EndIntermediateCommand(CommandBuffer);
    }

    void CreateImageView(VkImage Image, VkFormat Format, VkImageView &ImageView) {
        VkImageViewCreateInfo ViewCreateInfo{};
        ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ViewCreateInfo.image = Image;
        ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ViewCreateInfo.format = Format;
        ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ViewCreateInfo.subresourceRange.levelCount = 1;
        ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        ViewCreateInfo.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView(GetVKDevice(), &ViewCreateInfo, nullptr, &ImageView));
    }

    void CopyBufferToTexture(VkImage Image, VkBuffer Buffer, uint Width, uint Height) {
        VkCommandBuffer CommandBuffer = VulkanBackendApp::GetApplication()->BeginIntermediateCommand();

        VkBufferImageCopy Region{};
        Region.bufferOffset = 0;
        Region.bufferRowLength = 0;
        Region.bufferImageHeight = 0;

        Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        Region.imageSubresource.mipLevel = 0;
        Region.imageSubresource.baseArrayLayer = 0;
        Region.imageSubresource.layerCount = 1;

        Region.imageExtent = {
                Width, Height, 1
        };
        Region.imageOffset = {
                0, 0, 0
        };

        vkCmdCopyBufferToImage(CommandBuffer, Buffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &Region);

        VulkanBackendApp::GetApplication()->EndIntermediateCommand(CommandBuffer);
    }

    void GenerateMips(VkImage Image, uint Width, uint Height, uint NumMips) {
        auto CommandBuffer = VulkanBackendApp::GetApplication()->BeginIntermediateCommand();

        VkImageMemoryBarrier Barrier{};
        Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        Barrier.image = Image;
        Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        Barrier.subresourceRange.levelCount = 1;
        Barrier.subresourceRange.baseArrayLayer = 0;
        Barrier.subresourceRange.layerCount = 1;

        int SrcMipWidth = static_cast<int>(Width), SrcMipHeight = static_cast<int>(Height);
        for (int i = 1; i < NumMips; i++) {
            // Transition Src to TRANSFER_SRC_OPTIMAL
            Barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            Barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            Barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            Barrier.subresourceRange.baseMipLevel = i - 1;
            vkCmdPipelineBarrier(CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                                 1, &Barrier);

            VkImageBlit Blit{};
            Blit.srcOffsets[0] = {0, 0, 0};
            Blit.srcOffsets[1] = {SrcMipWidth, SrcMipHeight, 1};
            Blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            Blit.srcSubresource.mipLevel = i - 1;
            Blit.srcSubresource.baseArrayLayer = 0;
            Blit.srcSubresource.layerCount = 1;
            Blit.dstOffsets[0] = {0, 0, 0};
            int DstMipWidth = std::max(SrcMipWidth >> 1, 1);
            int DstMipHeight = std::max(SrcMipHeight >> 1, 1);
            Blit.dstOffsets[1] = {DstMipWidth, DstMipHeight, 1};
            Blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            Blit.dstSubresource.mipLevel = i;
            Blit.dstSubresource.baseArrayLayer = 0;
            Blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(CommandBuffer,
                           Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &Blit, VK_FILTER_LINEAR);

            SrcMipWidth = DstMipWidth;
            SrcMipHeight = DstMipHeight;

            // Transition Src to SHADER_READ_ONLY_OPTIMAL
            Barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            Barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            Barrier.subresourceRange.baseMipLevel = i - 1;
            vkCmdPipelineBarrier(CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                                 1, &Barrier);
        }

        // Transition Last Level to SHADER_READ_ONLY_OPTIMAL
        Barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        Barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        Barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        Barrier.subresourceRange.baseMipLevel = NumMips - 1;
        vkCmdPipelineBarrier(CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                             1, &Barrier);

        VulkanBackendApp::GetApplication()->EndIntermediateCommand(CommandBuffer);
    }

    void GenerateMips(VkImage Image, uint Width, uint Height, uint NumMips, VkFormat Format) {
        VkFormatProperties FormatProperties;
        vkGetPhysicalDeviceFormatProperties(GetVKPhysicalDevice(), Format, &FormatProperties);

        if (!(FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("Texture image format does not support linear blitting!");
        }

        // TODO
    }

}  // namespace HWPT::RHI
