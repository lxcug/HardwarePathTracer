//
// Created by HUSTLX on 2024/10/21.
//

#ifndef HARDWAREPATHTRACER_SWAPCHAIN_H
#define HARDWAREPATHTRACER_SWAPCHAIN_H

#include "core/Core.h"
#include <vector>
#include <optional>


namespace HWPT {
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR Capabilities;
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    struct QueueFamilyIndices {
        std::optional<uint> GraphicsFamily;
        std::optional<uint> ComputeFamily;
        std::optional<uint> PresentFamily;

        [[nodiscard]] auto IsComplete() const -> bool {
            return GraphicsFamily.has_value() && ComputeFamily.has_value() &&
                   PresentFamily.has_value();
        }
    };

    class SwapChain {
    public:
        explicit SwapChain(VkSurfaceKHR Surface, uint ConcurrentFrames = 1);

        ~SwapChain();

        void Create();

        void Destroy();

        void OnResize(uint Width, uint Height);

        static auto QuerySwapChainSupport(VkPhysicalDevice PhysicalDevice,
                                          VkSurfaceKHR Surface) -> SwapChainSupportDetails;

        static auto FindQueueFamilies(VkPhysicalDevice PhysicalDevice,
                                      VkSurfaceKHR Surface) -> QueueFamilyIndices;

        auto GetHandle() -> VkSwapchainKHR & {
            return m_swapChain;
        }

        [[nodiscard]] auto GetHandle() const -> const VkSwapchainKHR & {
            return m_swapChain;
        }

        [[nodiscard]] auto GetExtent() const -> VkExtent2D {
            return m_extent;
        }

        [[nodiscard]] auto GetConcurrentFrames() const -> uint {
            return m_concurrentFrames;
        }

        auto GetSwapChainImageViews() {
            return m_imageViews;
        }

        [[nodiscard]] auto GetFormat() const {
            return m_format;
        }

    private:
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        uint m_concurrentFrames = 1;
        VkSwapchainKHR m_swapChain;
        VkExtent2D m_extent;
        VkFormat m_format;
        std::vector<VkImage> m_images;
        std::vector<VkImageView> m_imageViews;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_SWAPCHAIN_H
