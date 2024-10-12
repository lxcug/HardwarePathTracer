//
// Created by HUSTLX on 2024/10/8.
//

#include "StorageBuffer.h"
#include "core/RHI.h"
#include "core/application/VulkanBackendApp.h"


namespace HWPT {

    StorageBuffer::StorageBuffer(VkDeviceSize Size, void *Data) {
        auto [StagingBuffer, StagingBufferMemory] = RHI::CreateStagingBuffer(Size);

        void* MappedData;
        vkMapMemory(GetVKDevice(), StagingBufferMemory, 0, Size, 0, &MappedData);
        memcpy(MappedData, Data, Size);
        vkUnmapMemory(GetVKDevice(), StagingBufferMemory);

        RHI::CreateBuffer(Size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_storageBuffer, m_storageBufferMemory);
        RHI::CopyBuffer(StagingBuffer, m_storageBuffer, Size);

        vkDestroyBuffer(GetVKDevice(), StagingBuffer, nullptr);
        vkFreeMemory(GetVKDevice(), StagingBufferMemory, nullptr);
    }

    StorageBuffer::~StorageBuffer() {
        vkFreeMemory(GetVKDevice(), m_storageBufferMemory, nullptr);
        vkDestroyBuffer(GetVKDevice(), m_storageBuffer, nullptr);
    }


}
