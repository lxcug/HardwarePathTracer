//
// Created by HUSTLX on 2025/1/6.
//

#include "ArbitraryBuffer.h"
#include "core/RHI.h"


namespace HWPT {

    ArbitraryBuffer::ArbitraryBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage,
                                     VkMemoryPropertyFlags MemProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        RHI::CreateBuffer(Size, Usage, MemProps, m_buffer, m_bufferMemory);
    }

    ArbitraryBuffer::ArbitraryBuffer(VkDeviceSize Size, void *Data, VkBufferUsageFlags Usage,
                                     VkMemoryPropertyFlags MemProps) {
        auto [StagingBuffer, StagingBufferMemory] = RHI::CreateStagingBuffer(Size);

        void *MappedData;
        vkMapMemory(GetVKDevice(), StagingBufferMemory, 0, Size, 0, &MappedData);
        memcpy(MappedData, Data, Size);
        vkUnmapMemory(GetVKDevice(), StagingBufferMemory);

        RHI::CreateBuffer(Size, Usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, MemProps, m_buffer,
                          m_bufferMemory);
        RHI::CopyBuffer(StagingBuffer, m_buffer, Size);

        vkDestroyBuffer(GetVKDevice(), StagingBuffer, nullptr);
        vkFreeMemory(GetVKDevice(), StagingBufferMemory, nullptr);
    }

    ArbitraryBuffer::~ArbitraryBuffer() {
        vkFreeMemory(GetVKDevice(), m_bufferMemory, nullptr);
        vkDestroyBuffer(GetVKDevice(), m_buffer, nullptr);
    }

}  // namespace HWPT
