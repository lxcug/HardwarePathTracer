//
// Created by HUSTLX on 2024/10/21.
//

#include "SwapChain.h"
#include <algorithm>
#include <unordered_set>


namespace HWPT {
    SwapChain::SwapChain(VkSurfaceKHR Surface, uint ConcurrentFrames)
            : m_surface(Surface), m_concurrentFrames(ConcurrentFrames) {
        Create();
    }

    SwapChain::~SwapChain() {
        Destroy();
    }

    void SwapChain::Create() {
        SwapChainSupportDetails SwapChainSupport = QuerySwapChainSupport(GetVKPhysicalDevice(),
                                                                         m_surface);

        VkSurfaceFormatKHR SurfaceFormat;
        for (auto Format: SwapChainSupport.Formats) {
            if (Format.format == VK_FORMAT_R8G8B8A8_SRGB &&
                Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                SurfaceFormat = Format;
                break;
            }
        }
        VkPresentModeKHR PresentMode;
        for (auto PreMode: SwapChainSupport.PresentModes) {
            if (PreMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                PresentMode = PreMode;
                break;
            }
        }
        VkExtent2D Extent = SwapChainSupport.Capabilities.currentExtent;
        uint ImageCount = std::clamp(SwapChainSupport.Capabilities.minImageCount,
                                     SwapChainSupport.Capabilities.minImageCount,
                                     SwapChainSupport.Capabilities.maxImageCount);
        Check(m_concurrentFrames >= ImageCount);

        VkSwapchainCreateInfoKHR CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        CreateInfo.surface = m_surface;
        CreateInfo.minImageCount = ImageCount;
        CreateInfo.imageFormat = SurfaceFormat.format;
        CreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
        CreateInfo.presentMode = PresentMode;
        CreateInfo.imageExtent = Extent;
        CreateInfo.imageArrayLayers = 1;
        CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        QueueFamilyIndices Indices = FindQueueFamilies(GetVKPhysicalDevice(), m_surface);
        std::unordered_set<uint> QueueFamilySet = {
                Indices.GraphicsFamily.value(),
                Indices.ComputeFamily.value(),
                Indices.PresentFamily.value()
        };
        if (QueueFamilySet.size() == 1) {
            CreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        } else {
            CreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            CreateInfo.queueFamilyIndexCount = QueueFamilySet.size();
            std::vector<uint> QueueIndices(QueueFamilySet.size());
            for (auto Index: QueueFamilySet) {
                QueueIndices.push_back(Index);
            }
            CreateInfo.pQueueFamilyIndices = QueueIndices.data();
        }

        CreateInfo.preTransform = SwapChainSupport.Capabilities.currentTransform;
        CreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        CreateInfo.clipped = VK_TRUE;
        CreateInfo.oldSwapchain = VK_NULL_HANDLE;

        VK_CHECK(vkCreateSwapchainKHR(GetVKDevice(), &CreateInfo, nullptr, &m_swapChain));
        m_extent = Extent;
        m_format = SurfaceFormat.format;

        uint ImgCount = 0;
        vkGetSwapchainImagesKHR(GetVKDevice(), m_swapChain, &ImgCount, nullptr);
        m_images.resize(ImageCount);
        vkGetSwapchainImagesKHR(GetVKDevice(), m_swapChain, &ImgCount, m_images.data());

        m_imageViews.resize(m_images.size());
        for (size_t Index = 0; Index < m_imageViews.size(); Index++) {
            VkImageViewCreateInfo ViewCreateInfo{};
            ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ViewCreateInfo.image = m_images[Index];
            ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ViewCreateInfo.format = m_format;
            ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ViewCreateInfo.subresourceRange.levelCount = 1;
            ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            ViewCreateInfo.subresourceRange.layerCount = 1;
            VK_CHECK(vkCreateImageView(GetVKDevice(), &ViewCreateInfo, nullptr,
                                       &m_imageViews[Index]));
        }
    }

    void SwapChain::Destroy() {
        for (int i = 0; i < m_concurrentFrames; i++) {
            vkDestroyImageView(GetVKDevice(), m_imageViews[i], nullptr);
        }
        vkDestroySwapchainKHR(GetVKDevice(), m_swapChain, nullptr);
    }

    void SwapChain::OnResize(uint Width, uint Height) {
        Destroy();

        m_extent = {Width, Height};
        Create();
    }

    auto SwapChain::QuerySwapChainSupport(VkPhysicalDevice PhysicalDevice,
                                          VkSurfaceKHR Surface) -> SwapChainSupportDetails {
        SwapChainSupportDetails Details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, Surface, &Details.Capabilities);

        uint FormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface, &FormatCount, nullptr);
        if (FormatCount > 0) {
            Details.Formats.resize(FormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, Surface,
                                                 &FormatCount, Details.Formats.data());
        }

        uint PresentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface, &PresentModeCount,
                                                  nullptr);

        if (PresentModeCount > 0) {
            Details.PresentModes.resize(PresentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, Surface,
                                                      &PresentModeCount,
                                                      Details.PresentModes.data());
        }

        return Details;
    }

    auto SwapChain::FindQueueFamilies(VkPhysicalDevice PhysicalDevice,
                                      VkSurfaceKHR Surface) -> QueueFamilyIndices {
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
            vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, Index, Surface, &PresentSupport);
            if (PresentSupport) {
                Indices.PresentFamily = Index;
            }
            if (Indices.IsComplete()) {
                break;
            }
            ++Index;
        }

        return Indices;
    }
}  // namespace HWPT
