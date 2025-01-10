//
// Created by HUSTLX on 2025/1/7.
//

#include "Utils.h"
#include "core/application/VulkanBackendApp.h"
#include "core/Commands/CommandPool.h"
#include <vector>


namespace HWPT::Utils {

    auto FindQueueFamilies(VkPhysicalDevice PhysicalDevice) -> QueueFamilyIndices {
        QueueFamilyIndices Indices;

        uint QueueFamilyCount = 0;

        vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount,
                                                 QueueFamilies.data());

        int Index = 0;
        for (const auto &QueueFamily: QueueFamilies) {
            if (QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                Indices.GraphicsFamily = Index;
            }
            // TODO: Use Dedicated Compute Queue, !(QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            if (QueueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                Indices.ComputeFamily = Index;
            }

            VkBool32 PresentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, Index,
                                                 VulkanBackendApp::GetApplication()->GetSurface(),
                                                 &PresentSupport);
            if (PresentSupport) {
                Indices.PresentFamily = Index;
            }
            if (Indices.IsComplete()) {
                break;
            }
            Index++;
        }

        return Indices;
    }

    auto FindQueueFamilies() -> QueueFamilyIndices {
        return FindQueueFamilies(GetVKPhysicalDevice());
    }

    auto GLMToVulkanMatrix(const glm::mat4& Mat) -> VkTransformMatrixKHR {
        VkTransformMatrixKHR RetMat;
        memcpy(&RetMat, glm::value_ptr(glm::transpose(Mat)), sizeof(VkTransformMatrixKHR));
        return RetMat;
    }

    auto Align(uint Size, uint Alignment) -> uint {
        return (Size + Alignment - 1) & ~(Alignment - 1);
    }

}  // namespace HWPT::Utils
