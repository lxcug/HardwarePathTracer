//
// Created by HUSTLX on 2025/1/7.
//

#ifndef HARDWAREPATHTRACER_UTILS_H
#define HARDWAREPATHTRACER_UTILS_H

#include "core/Core.h"
#include <type_traits>


namespace HWPT {
    struct QueueFamilyIndices;
}  // namespace HWPT

namespace HWPT::Utils {

    template<typename EnumClassType>
    auto HasFlag(EnumClassType Enum, EnumClassType Flag) -> bool {
        return Enum & Flag;
//        using underlying = std::underlying_type_t<EnumClassType>;
//        return (static_cast<underlying>(Enum) & static_cast<underlying>(Flag)) != 0;
    }

    auto FindQueueFamilies(VkPhysicalDevice PhysicalDevice) -> QueueFamilyIndices;

    auto FindQueueFamilies() -> QueueFamilyIndices;

    auto GLMToVulkanMatrix(const glm::mat4& Mat) -> VkTransformMatrixKHR;

    auto Align(uint Size, uint Alignment) -> uint;

    auto Float3ArrayToGLM(const float Value[3]) -> glm::vec3;
}  // namespace HWPT::Utils

#endif //HARDWAREPATHTRACER_UTILS_H
