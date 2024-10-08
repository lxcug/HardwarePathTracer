//
// Created by HUSTLX on 2024/10/7.
//

#ifndef HARDWAREPATHTRACER_VULKANBACKENDAPP_H
#define HARDWAREPATHTRACER_VULKANBACKENDAPP_H

#include <vulkan/vulkan.h>
#include "GLFW/glfw3.h"
#include "Application.h"
#include <string>
#include <iostream>
#include <vector>
#include <optional>
#include <functional>
#include "core/buffer/VertexBuffer.h"
#include "core/buffer/IndexBuffer.h"
#include "core/buffer/UniformBuffer.h"
#include "core/buffer/StorageBuffer.h"


namespace HWPT {
    const int MAX_FRAMES_IN_FLIGHT = 2;

    struct QueueFamilyIndices {
        std::optional<uint> GraphicsFamily;
        std::optional<uint> ComputeFamily;
        std::optional<uint> PresentFamily;

        [[nodiscard]] bool IsComplete() const {
            return GraphicsFamily.has_value() && ComputeFamily.has_value() && PresentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR Capabilities;
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    struct QueueHandle {
        VkQueue GraphicsQueue = VK_NULL_HANDLE;
        VkQueue ComputeQueue = VK_NULL_HANDLE;
        VkQueue PresentQueue = VK_NULL_HANDLE;
    };

    struct SwapChain {
        VkSwapchainKHR SwapChainHandle = VK_NULL_HANDLE;
        VkExtent2D Extent;
        VkFormat Format;
        std::vector<VkImage> SwapChainImages;
        std::vector<VkImageView> SwapChainImageViews;

        void CreateImageViews(VkDevice Device);

        void GetImages(VkDevice Device);
    };

    struct CommandPool {
        VkCommandPool GraphicsPool = VK_NULL_HANDLE;
        VkCommandPool ComputePool = VK_NULL_HANDLE;
    };

    class VulkanBackendApp : public ApplicationBase {
    public:
        void Run() override;

        void Init() override;

        void DrawFrame();

        VkCommandBuffer BeginIntermediateCommand();

        void EndIntermediateCommand(VkCommandBuffer commandBuffer);

        VkInstance GetVkInstance() {
            return m_instance;
        }

        VkDevice GetVkDevice() {
            return m_device;
        }

        VkPhysicalDevice GetPhysicalDevice() {
            return m_physicalDevice;
        }

        static VulkanBackendApp* GetApplication() {
            return g_application;
        }

        static VkDevice GetGlobalDevice() {
            return GetApplication()->GetVkDevice();
        }

    private:
        // Init GLFW Windows
        void InitWindow();

        // Init Vulkan Backend
        void InitVulkan();

        void CleanUp();

        // FrameBuffer Resize Callback
        static void FrameBufferResizeCallback(GLFWwindow* Window, int Width, int Height);

    private:
        // VulkanContext Init
        void CreateVkInstance();

        static std::vector<const char*> GetRequiredExtensions();

        void CreateSurface();

        void SelectPhysicalDevice();

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice PhysicalDevice);

        static bool IsDeviceExtensionSupport(VkPhysicalDevice PhysicalDevice);

        SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice PhysicalDevice);

        bool IsSuitableDevice(VkPhysicalDevice PhysicalDevice);

        void CreateLogicalDevice();

        void CreateSwapChain();

        void CleanUpSwapChain();

        void CreateRenderPass();

        void CreateFrameBuffers();

        void CreateCommandPool();

        void CreateCommandBuffers();

        void CreateGraphicsDescriptorSetLayout();

        void CreateGraphicsPipeline();

        void CreateComputeDescriptorSetLayout();

        void CreateComputePipeline();

    private:
        std::string m_windowTitle = "VulkanBackend Application";
        uint m_windowWidth = 1600, m_windowHeight = 900;
        GLFWwindow* m_window = nullptr;
        bool m_frameBufferResized = false;
        bool m_contextInited = false;

#if !BUILD_RELEASE && !BUILD_SHIPPING
        inline static bool m_enableValidationLayers = true;
        inline static const std::vector<const char *> ValidationLayers = {
                "VK_LAYER_KHRONOS_validation"
        };
#else
        inline static bool m_enableValidationLayers = false;
        inline static const std::vector<const char *> ValidationLayers = {};
#endif
        inline static std::vector<const char *> DeviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                VK_NV_RAY_TRACING_EXTENSION_NAME
        };

        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        QueueHandle m_queue;
        SwapChain m_swapChain;
        VkRenderPass m_renderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> SwapChainFrameBuffers;
        CommandPool m_commandPool;
        std::vector<VkCommandBuffer> m_graphicsCommandBuffers;
        std::vector<VkCommandBuffer> m_computeCommandBuffers;
        // Graphics Pipeline
        VkDescriptorSetLayout m_graphicsDescriptorSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout m_graphicsPipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
        // Compute Pipeline
        VkDescriptorSetLayout m_computeDescriptorSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout m_computePipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_computePipeline = VK_NULL_HANDLE;

        uint m_currentFrame = 0;

        inline static VulkanBackendApp* g_application = nullptr;
    };
}

#endif //HARDWAREPATHTRACER_VULKANBACKENDAPP_H
