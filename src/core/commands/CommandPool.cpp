//
// Created by HUSTLX on 2025/1/7.
//

#include "CommandPool.h"
#include <vector>
#include "core/application/VulkanBackendApp.h"
#include "core/Utils.h"


namespace Shadowy {

    void CommandPool::Init() {
        Check(Utils::HasFlag(m_poolType, PoolType::Graphics) ||
              Utils::HasFlag(m_poolType, PoolType::Compute) &&
              "CommandPool should at least have one of the following PoolTypes: Graphics or Compute");

        m_queueFamilyIndices = Utils::FindQueueFamilies();

        VkCommandPoolCreateInfo CreateInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        CreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        if (Utils::HasFlag(m_poolType, PoolType::Transient)) {
            CreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        }
        if (Utils::HasFlag(m_poolType, PoolType::Protected)) {
            CreateInfo.flags |= VK_COMMAND_POOL_CREATE_PROTECTED_BIT;
        }

        if (Utils::HasFlag(m_poolType, PoolType::Graphics)) {

            CreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            Check(m_queueFamilyIndices.GraphicsFamily.has_value());
            CreateInfo.queueFamilyIndex = m_queueFamilyIndices.GraphicsFamily.value();

            VK_CHECK(vkCreateCommandPool(GetVKDevice(), &CreateInfo, nullptr, &m_graphicsPool));

            vkGetDeviceQueue(GetVKDevice(), m_queueFamilyIndices.GraphicsFamily.value(), 0,
                             &m_graphicsQueue);
            vkGetDeviceQueue(GetVKDevice(), m_queueFamilyIndices.PresentFamily.value(), 0,
                             &m_presentQueue);
        }

        if (Utils::HasFlag(m_poolType, PoolType::Compute)) {
            Check(m_queueFamilyIndices.ComputeFamily.has_value());
            CreateInfo.queueFamilyIndex = m_queueFamilyIndices.ComputeFamily.value();

            VK_CHECK(vkCreateCommandPool(GetVKDevice(), &CreateInfo, nullptr, &m_computePool));

            vkGetDeviceQueue(GetVKDevice(), m_queueFamilyIndices.ComputeFamily.value(), 0,
                             &m_computeQueue);
        }
    }

    CommandPool::~CommandPool() {
        if (HasGraphicsCommandPool()) {
            vkDestroyCommandPool(GetVKDevice(), m_graphicsPool, nullptr);
        }
        if (HasComputeCommandPool()) {
            vkDestroyCommandPool(GetVKDevice(), m_computePool, nullptr);
        }
    }

    auto CommandPool::BeginCommandBuffer(QueueType QueueType_) -> VkCommandBuffer {
        VkCommandBufferAllocateInfo AllocateInfo{};
        AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        AllocateInfo.commandBufferCount = 1;

        // We don't begin a CommandBuffer on PresentQueue explicitly at all ?
        if (QueueType_ == QueueType::Graphics) {
            Check(HasGraphicsCommandPool());
            AllocateInfo.commandPool = m_graphicsPool;

        } else if (QueueType_ == QueueType::Compute) {
            Check(HasComputeCommandPool());
            AllocateInfo.commandPool = m_computePool;
        }

        VkCommandBuffer CommandBuffer;
        VK_CHECK(vkAllocateCommandBuffers(GetVKDevice(), &AllocateInfo, &CommandBuffer));

        VkCommandBufferBeginInfo BeginInfo{};
        BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(CommandBuffer, &BeginInfo);

        return CommandBuffer;
    }

    void CommandPool::SubmitCommandBuffer(VkCommandBuffer CommandBuffer, QueueType QueueType_) {
        vkEndCommandBuffer(CommandBuffer);

        VkSubmitInfo SubmitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        SubmitInfo.commandBufferCount = 1;
        SubmitInfo.pCommandBuffers = &CommandBuffer;

        if (QueueType_ == QueueType::Graphics) {
            Check(HasGraphicsCommandPool());
            VkResult Result = vkQueueSubmit(m_graphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
            Result = vkQueueWaitIdle(m_graphicsQueue);
            vkFreeCommandBuffers(GetVKDevice(), m_graphicsPool, 1, &CommandBuffer);
        } else if (QueueType_ == QueueType::Compute) {
            Check(HasComputeCommandPool());
            VkResult Result = vkQueueSubmit(m_computeQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
            Result = vkQueueWaitIdle(m_computeQueue);
            vkFreeCommandBuffers(GetVKDevice(), m_computePool, 1, &CommandBuffer);
        }
    }

    auto CommandPool::HasGraphicsCommandPool() const -> bool {
        return m_graphicsPool != VK_NULL_HANDLE && Utils::HasFlag(m_poolType, PoolType::Graphics);
    }

    auto CommandPool::HasComputeCommandPool() const -> bool {
        return m_computePool != VK_NULL_HANDLE && Utils::HasFlag(m_poolType, PoolType::Compute);
    }

}  // namespace Shadowy

