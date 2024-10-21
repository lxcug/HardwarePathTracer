//
// Created by HUSTLX on 2024/10/7.
//

#include "IndexBuffer.h"
#include "core/RHI.h"
#include "core/application/VulkanBackendApp.h"


namespace HWPT {

    IndexBuffer::IndexBuffer(uint IndexCount, const void *Data): m_indexCount(IndexCount) {
        VkDeviceSize Size = IndexCount * sizeof(uint);
        auto [StagingBuffer, StagingBufferMemory] = RHI::CreateStagingBuffer(Size);

        void *MappedData = nullptr;
        vkMapMemory(GetVKDevice(), StagingBufferMemory, 0, Size, 0, &MappedData);
        memcpy(MappedData, Data, Size);
        vkUnmapMemory(GetVKDevice(), StagingBufferMemory);

        RHI::CreateBuffer(Size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          m_indexBuffer, m_indexBufferMemory);

        RHI::CopyBuffer(StagingBuffer, m_indexBuffer, Size);

        vkDestroyBuffer(GetVKDevice(), StagingBuffer, nullptr);
        vkFreeMemory(GetVKDevice(), StagingBufferMemory, nullptr);
    }

    IndexBuffer::~IndexBuffer() {
        vkFreeMemory(GetVKDevice(), m_indexBufferMemory, nullptr);
        vkDestroyBuffer(GetVKDevice(), m_indexBuffer, nullptr);
    }

    void IndexBuffer::Bind(VkCommandBuffer CommandBuffer) const {
        vkCmdBindIndexBuffer(CommandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }

    IndexBuffer::IndexBuffer(uint IndexCount, const uint *Data)
    : IndexBuffer(IndexCount, static_cast<const void*>(Data)) {}
}  // namespace HWPT
