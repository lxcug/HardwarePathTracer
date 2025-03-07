//
// Created by HUSTLX on 2025/1/6.
//

#include "ArbitraryBuffer.h"
#include "core/RHI.h"


namespace Shadowy {

    ArbitraryBuffer::ArbitraryBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage,
                                     VkMemoryPropertyFlags MemProps): m_size(Size) {
        Size = std::max(Size, static_cast<uint64_t>(1));
        RHI::CreateBuffer(Size, Usage, MemProps, m_buffer, m_bufferMemory);
    }

    ArbitraryBuffer::ArbitraryBuffer(VkDeviceSize Size, void *Data, VkBufferUsageFlags Usage,
                                     VkMemoryPropertyFlags MemProps): m_size(Size) {
        Size = std::max(Size, static_cast<uint64_t>(1));
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

    void ArbitraryBuffer::Update(VkDeviceSize Size, void *Data) {
        Check(m_size > 0);
        Size = std::max(Size, static_cast<uint64_t>(1));
        auto [StagingBuffer, StagingBufferMemory] = RHI::CreateStagingBuffer(Size);

        void *MappedData;
        vkMapMemory(GetVKDevice(), StagingBufferMemory, 0, Size, 0, &MappedData);
        memcpy(MappedData, Data, Size);
        vkUnmapMemory(GetVKDevice(), StagingBufferMemory);

        RHI::CopyBuffer(StagingBuffer, m_buffer, Size);

        vkDestroyBuffer(GetVKDevice(), StagingBuffer, nullptr);
        vkFreeMemory(GetVKDevice(), StagingBufferMemory, nullptr);
    }

    void ArbitraryBuffer::Update(void *Data) {
        Update(m_size, Data);
    }

    void ArbitraryBuffer::ReadBuffer(void *&OutData) {
        vkMapMemory(GetVKDevice(), m_bufferMemory, 0, VK_WHOLE_SIZE, 0, &OutData);
        vkUnmapMemory(GetVKDevice(), m_bufferMemory);
    }

}  // namespace Shadowy
