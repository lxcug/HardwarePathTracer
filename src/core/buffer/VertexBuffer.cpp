//
// Created by HUSTLX on 2024/10/7.
//

#include "VertexBuffer.h"
#include "core/application/VulkanBackendApp.h"
#include "core/RHI.h"


namespace HWPT {

    VertexBuffer::VertexBuffer(VkDeviceSize Size, void *Data) {
        auto [StagingBuffer, StagingBufferMemory] = RHI::CreateStagingBuffer(Size);

        void* MappedData = nullptr;
        vkMapMemory(VulkanBackendApp::GetGlobalDevice(), StagingBufferMemory, 0, Size, 0, &MappedData);
        memcpy(MappedData, Data, Size);
        vkUnmapMemory(VulkanBackendApp::GetGlobalDevice(), StagingBufferMemory);

        RHI::CreateBuffer(Size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     m_vertexBuffer, m_vertexBufferMemory);

        RHI::CopyBuffer(StagingBuffer, m_vertexBuffer, Size);

        vkDestroyBuffer(VulkanBackendApp::GetGlobalDevice(), StagingBuffer, nullptr);
        vkFreeMemory(VulkanBackendApp::GetGlobalDevice(), StagingBufferMemory, nullptr);
    }

    VertexBuffer::~VertexBuffer() {
        vkFreeMemory(VulkanBackendApp::GetGlobalDevice(), m_vertexBufferMemory, nullptr);
        vkDestroyBuffer(VulkanBackendApp::GetGlobalDevice(), m_vertexBuffer, nullptr);
    }

    VkVertexInputBindingDescription Vertex::GetBindingDescription() {
        VkVertexInputBindingDescription BindingDescription{};

        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(Vertex);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    std::array<VkVertexInputAttributeDescription, 2> Vertex::GetAttributeDescriptions() {
        VkVertexInputAttributeDescription Attr0, Attr1;
        Attr0.binding = 0;
        Attr0.location = 0;
        Attr0.format = VK_FORMAT_R32G32B32_SFLOAT;
        Attr0.offset = offsetof(Vertex, Pos);

        Attr1.binding = 0;
        Attr1.location = 1;
        Attr1.format = VK_FORMAT_R32G32B32_SFLOAT;
        Attr1.offset = offsetof(Vertex, Color);

        return {Attr0, Attr1};
    }
}