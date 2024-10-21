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
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          m_vertexBuffer, m_vertexBufferMemory);

        RHI::CopyBuffer(StagingBuffer, m_vertexBuffer, Size);

        vkDestroyBuffer(GetVKDevice(), StagingBuffer, nullptr);
        vkFreeMemory(GetVKDevice(), StagingBufferMemory, nullptr);

        m_vertexCount = Size / sizeof(Vertex);
    }

    VertexBuffer::VertexBuffer(VkDeviceSize Size, const Vertex *Data)
            : VertexBuffer(Size, static_cast<const void *>(Data)) {}

    VertexBuffer::~VertexBuffer() {
        delete m_layout;
        vkFreeMemory(GetVKDevice(), m_vertexBufferMemory, nullptr);
        vkDestroyBuffer(GetVKDevice(), m_vertexBuffer, nullptr);
    }

    void VertexBuffer::Bind(VkCommandBuffer CommandBuffer) const {
        VkDeviceSize Offset = 0;
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &m_vertexBuffer, &Offset);
    }

    void VertexBuffer::SetLayout(const std::initializer_list<VertexAttribute> &Attributes) {
        Check(m_layout == nullptr);

        m_layout = new VertexBufferLayout(Attributes);
    }

    auto Vertex::GetBindingDescription() -> VkVertexInputBindingDescription {
        VkVertexInputBindingDescription BindingDescription{};

        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(Vertex);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    auto Vertex::GetAttributeDescriptions() -> std::array<VkVertexInputAttributeDescription, 3> {
        std::array<VkVertexInputAttributeDescription, 3> AttributeDescriptions{};
        AttributeDescriptions[0].binding = 0;
        AttributeDescriptions[0].location = 0;
        AttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[0].offset = offsetof(Vertex, Pos);

        AttributeDescriptions[1].binding = 0;
        AttributeDescriptions[1].location = 1;
        AttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        AttributeDescriptions[1].offset = offsetof(Vertex, Color);

        AttributeDescriptions[2].binding = 0;
        AttributeDescriptions[2].location = 2;
        AttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        AttributeDescriptions[2].offset = offsetof(Vertex, TexCoord);

        return AttributeDescriptions;
    }
}  // namespace HWPT
