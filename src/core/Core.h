//
// Created by HUSTLX on 2024/10/7.
//

#ifndef HARDWAREPATHTRACER_CORE_H
#define HARDWAREPATHTRACER_CORE_H

#include <cstdint>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.h>
// Math
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define BUILD_DEVELOP 1
#define BUILD_SHIPPING 0
#define BUILD_RELEASE 0

// NOTE: Should be implemented in Application
namespace HWPT {
    VkDevice GetVKDevice();

    VkPhysicalDevice GetVKPhysicalDevice();
}

#if BUILD_DEVELOP
#define BUILD_SHIPPING 0
#define BUILD_RELEASE 0
#endif

using uint = uint32_t;

#if !BUILD_RELEASE && !BUILD_SHIPPING
#define Assert(x) assert(x)
#define Check(x) assert(x)
#else
#define Assert(x)
#define Check(x)
#endif

#if !BUILD_RELEASE && !BUILD_SHIPPING
#define VK_CHECK(exp) do { VkResult _Result_ = exp; \
Check(_Result_ == VK_SUCCESS); } while(false)

#define VK_CHECK_WITH_MESSAGE(exp, message) do { VkResult _Result_ = exp; \
if (_Result_ != VK_SUCCESS) \
    throw std::runtime_error(message); } while(false)
#else
#define VK_CHECK(exp) exp
#define VK_CHECK_WITH_MESSAGE(exp, message) exp
#endif


#endif //HARDWAREPATHTRACER_CORE_H
