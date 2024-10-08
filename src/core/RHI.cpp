//
// Created by HUSTLX on 2024/10/7.
//

#include "RHI.h"
#include "core/application/VulkanBackendApp.h"


namespace HWPT::RHI {

    uint FindMemoryType(uint TypeFilter, VkMemoryPropertyFlags Properties) {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(VulkanBackendApp::GetApplication()->GetPhysicalDevice(), &memoryProperties);

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

        VkDevice GlobalDevice = VulkanBackendApp::GetGlobalDevice();
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

    std::tuple<VkBuffer, VkDeviceMemory> CreateStagingBuffer(VkDeviceSize Size) {
        VkBuffer StagingBuffer;
        VkDeviceMemory StagingBufferMemory;

        CreateBuffer(Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     StagingBuffer, StagingBufferMemory);

        return std::make_tuple(StagingBuffer, StagingBufferMemory);
    }
}
