//
// Created by HUSTLX on 2025/1/7.
//

#ifndef HARDWAREPATHTRACER_UTILS_H
#define HARDWAREPATHTRACER_UTILS_H

#include "core/Core.h"
#include <type_traits>


namespace Shadowy {
    struct QueueFamilyIndices;
}  // namespace Shadowy

namespace Shadowy::Utils {

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
}  // namespace Shadowy::Utils

#endif //HARDWAREPATHTRACER_UTILS_H
