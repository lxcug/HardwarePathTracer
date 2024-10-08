//
// Created by HUSTLX on 2024/10/8.
//

#include "UniformBuffer.h"
#include "core/RHI.h"
#include "core/application/VulkanBackendApp.h"


namespace HWPT {

    UniformBuffer::UniformBuffer(VkDeviceSize Size, void *Data): m_size(Size) {
        RHI::CreateBuffer(Size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          m_uniformBuffer, m_uniformBufferMemory);

        vkMapMemory(VulkanBackendApp::GetGlobalDevice(), m_uniformBufferMemory, 0, Size, 0, &m_mappedData);
        memcpy(m_mappedData, Data, Size);
    }

    UniformBuffer::~UniformBuffer() {
        vkUnmapMemory(VulkanBackendApp::GetGlobalDevice(), m_uniformBufferMemory);
        vkFreeMemory(VulkanBackendApp::GetGlobalDevice(), m_uniformBufferMemory, nullptr);
        vkDestroyBuffer(VulkanBackendApp::GetGlobalDevice(), m_uniformBuffer, nullptr);
    }

    void UniformBuffer::Update(void *Data) {
        memcpy(m_mappedData, Data, m_size);
    }
}
