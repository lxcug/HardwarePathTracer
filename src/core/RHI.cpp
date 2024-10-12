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
                      VkBuffer& Buffer, VkDeviceMemory& BufferMemory) {
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
        allocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, Properties);

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

    void CreateTexture2D(uint Width, uint Height, VkFormat Format, VkImageUsageFlags Usage,
                         VkImageTiling Tiling, VkImage& Texture, VkDeviceMemory& TextureMemory) {
        VkImageCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        CreateInfo.imageType = VK_IMAGE_TYPE_2D;
        CreateInfo.extent.width = Width;
        CreateInfo.extent.height = Height;
        CreateInfo.extent.depth = 1;
        CreateInfo.mipLevels = 1;
        CreateInfo.arrayLayers = 1;
        CreateInfo.format = Format;
        CreateInfo.tiling = Tiling;
        CreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        CreateInfo.usage = Usage;
        CreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        CreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK(vkCreateImage(GetVKDevice(), &CreateInfo, nullptr, &Texture));

        VkMemoryRequirements MemRequirements;
        vkGetImageMemoryRequirements(GetVKDevice(), Texture, &MemRequirements);
        VkMemoryAllocateInfo AllocateInfo{};
        AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        AllocateInfo.allocationSize = MemRequirements.size;
        AllocateInfo.memoryTypeIndex = FindMemoryType(MemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(GetVKDevice(), &AllocateInfo, nullptr, &TextureMemory);
        vkBindImageMemory(GetVKDevice(), Texture, TextureMemory, 0);
    }

    void TransitionTextureLayout(VkImage Image, VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout) {
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
        Barrier.subresourceRange.levelCount = 1;
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
        } else {
            throw std::runtime_error("Unsupported layout transition!");
        }

        vkCmdPipelineBarrier(CommandBuffer, SourceStage, DestinationStage,
                             0, 0, nullptr, 0, nullptr, 1, &Barrier);

        VulkanBackendApp::GetApplication()->EndIntermediateCommand(CommandBuffer);
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

}  // namespace HWPT::RHI
