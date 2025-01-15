//
// Created by HUSTLX on 2024/10/7.
//

#include "VertexBuffer.h"
#include "core/application/VulkanBackendApp.h"
#include "core/RHI.h"


namespace HWPT {
    VertexBuffer::VertexBuffer(VkDeviceSize Size, const void *Data) {
        auto [StagingBuffer, StagingBufferMemory] = RHI::CreateStagingBuffer(Size);

        void *MappedData = nullptr;
        vkMapMemory(GetVKDevice(), StagingBufferMemory, 0, Size, 0, &MappedData);
        memcpy(MappedData, Data, Size);
        vkUnmapMemory(GetVKDevice(), StagingBufferMemory);

        RHI::CreateBuffer(Size,
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          m_vertexBuffer, m_vertexBufferMemory);

        RHI::CopyBuffer(StagingBuffer, m_vertexBuffer, Size);

        vkDestroyBuffer(GetVKDevice(), StagingBuffer, nullptr);
        vkFreeMemory(GetVKDevice(), StagingBufferMemory, nullptr);
    }

    VertexBuffer::VertexBuffer(VkDeviceSize Size, const Vertex *Data)
            : VertexBuffer(Size, static_cast<const void *>(Data)) {
        m_vertexCount = Size / sizeof(Vertex);
    }

    VertexBuffer::~VertexBuffer() {
        delete m_layout;
        vkFreeMemory(GetVKDevice(), m_vertexBufferMemory, nullptr);
        vkDestroyBuffer(GetVKDevice(), m_vertexBuffer, nullptr);
    }

    void VertexBuffer::Bind(VkCommandBuffer CommandBuffer) {
        VkDeviceSize Offset = 0;
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &m_vertexBuffer, &Offset);
    }

    void VertexBuffer::SetLayout(const std::initializer_list<VertexAttribute> &Attributes) {
        Check(m_layout == nullptr);

        m_layout = new VertexBufferLayout(Attributes);
    }
}  // namespace HWPT
