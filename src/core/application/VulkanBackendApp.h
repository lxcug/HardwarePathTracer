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
#include <chrono>
#include <optional>
#include <functional>
#include "core/buffer/VertexBuffer.h"
#include "core/buffer/IndexBuffer.h"
#include "core/buffer/UniformBuffer.h"
#include "core/buffer/StorageBuffer.h"
#include "core/FPSCalculator.h"
#include "core/texture/Texture2D.h"
#include "core/texture/Sampler.h"
#include "ImGuiIntegration.h"
#include "core/scene/Model.h"
#include "core/Commands/CommandPool.h"
#include "core/scene/Camera.h"
#include "core/debug/Debugger.h"
#include <host_device_shared/ViewUniformBuffer.h>


namespace Shadowy
{
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR Capabilities;
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    struct Queue
    {
        VkQueue GraphicsQueue = VK_NULL_HANDLE;
        VkQueue ComputeQueue = VK_NULL_HANDLE;
        VkQueue PresentQueue = VK_NULL_HANDLE;
    };

    struct SwapChain
    {
        VkSwapchainKHR SwapChainHandle = VK_NULL_HANDLE;
        VkExtent2D Extent;
        VkFormat Format;
        std::vector<VkImage> SwapChainImages;
        std::vector<VkImageView> SwapChainImageViews;

        void CreateImageViews(VkDevice Device);

        void GetImages(VkDevice Device);
    };

    struct MSAABuffer
    {
        Texture2D* MSAAColorBuffer = nullptr;
        Texture2D* MSAADepthBuffer = nullptr;

        ~MSAABuffer()
        {
            delete MSAAColorBuffer;
            delete MSAADepthBuffer;
        }
    };

    class VulkanBackendApp : public ApplicationBase
    {
    public:
        explicit VulkanBackendApp(const std::string& Title = "VulkanBackend Application"):
            m_windowTitle(Title)
        {
        }

        void Run() override;

        void Init() override;

        void DrawFrame() override;

        virtual void DrawImGuiFrame();

        virtual void ResetFrameNum()
        {
            m_accumulatedFrameNum = 0;
        }

        [[nodiscard]] auto GetVkInstance() const -> VkInstance
        {
            return m_instance;
        }

        [[nodiscard]] auto GetVkDevice() const -> VkDevice
        {
            return m_device;
        }

        [[nodiscard]] auto GetPhysicalDevice() const -> VkPhysicalDevice
        {
            return m_physicalDevice;
        }

        static auto GetApplication() -> VulkanBackendApp*
        {
            return s_application;
        }

        auto GetSwapChain() -> SwapChain
        {
            return m_swapChain;
        }

        [[nodiscard]] auto GetQueue() const -> Queue
        {
            return m_queue;
        }

        [[nodiscard]] auto GetFrameBuffers(uint Index) const -> VkFramebuffer
        {
            return m_swapChainFrameBuffers[Index];
        }

        [[nodiscard]] auto GetWindow() const -> GLFWwindow*
        {
            return m_window;
        }

        [[nodiscard]] auto GetImageIndex() const -> uint
        {
            return m_imageIndex;
        }

        [[nodiscard]] auto GetCommandPool() const -> CommandPool*
        {
            return m_commandPool;
        }

        [[nodiscard]] auto GetSurface() const -> VkSurfaceKHR
        {
            return m_surface;
        }

        [[nodiscard]] auto GetDescriptorPool() const -> VkDescriptorPool
        {
            return m_descriptorPool;
        }

        auto GetCamera() const -> std::shared_ptr<CameraBase>
        {
            return m_camera;
        }

    protected:
        // Init GLFW Windows
        void InitWindow();

        // Init Vulkan Backend
        virtual void InitVulkan();

        virtual void CleanUp();

        void Present();

        // FrameBuffer Resize Callback
        static void FrameBufferResizeCallback(GLFWwindow* Window, int Width, int Height);

        static void MouseScrollCallBack(GLFWwindow* Window, double XOffset, double YOffset);

        void RecreateSwapChain();

        void CreateMSAABuffers();

    protected:
        void InitImGui();

        void CleanUpImGui();

    protected:
        // VulkanContext Init
        void CreateVkInstance();

        static auto GetRequiredExtensions() -> std::vector<const char*>;

        void CreateSurface();

        void SelectPhysicalDevice();

        static auto IsDeviceExtensionSupport(VkPhysicalDevice PhysicalDevice) -> bool;

        auto QuerySwapChainSupport(VkPhysicalDevice PhysicalDevice) -> SwapChainSupportDetails;

        auto IsSuitableDevice(VkPhysicalDevice PhysicalDevice) -> bool;

        void CreateLogicalDevice();

        void CreateSwapChain();

        void CleanUpSwapChain();

        void CreateRenderPass();

        void CreateFrameBuffers();

        void CreateCommandPool();

        void CreateCommandBuffers();

        void CreateGraphicsDescriptorSetLayout();

        void CreateGraphicsPipeline();

        void CreateParticleGraphicsPipeline();

        void CreateComputeDescriptorSetLayout();

        void CreateComputePipeline();

        void CreateDescriptorPool();

        void CreateGraphicsDescriptorSets();

        void CreateComputeDescriptorSets();

        virtual void CreateSyncObjects();

        void RecordCommandBuffer(VkCommandBuffer CommandBuffer, uint ImageIndex);

        void CreateViewUniformBuffers();

        virtual void OnWindowResize();

    protected:
        VkDevice m_device = VK_NULL_HANDLE;

        std::string m_windowTitle;
        uint m_windowWidth = 2133, m_windowHeight = 1200;
        GLFWwindow* m_window = nullptr;
        bool m_frameBufferResized = false;
        bool m_contextInited = false;

        std::shared_ptr<FPSCalculator> m_fpsCalculator;

#if !BUILD_RELEASE && !BUILD_SHIPPING
        inline static bool m_enableValidationLayers = true;
        inline static const std::vector<const char*> ValidationLayers = {
            "VK_LAYER_KHRONOS_validation",
        };
#else
        inline static bool m_enableValidationLayers = false;
        inline static const std::vector<const char *> ValidationLayers = {};
#endif
        inline static std::vector<const char*> DeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
            // VK_EXT_DEBUG_MARKER_EXTENSION_NAME
            // VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        };

        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        Queue m_queue;
        SwapChain m_swapChain;
        std::vector<VkFramebuffer> m_swapChainFrameBuffers;
        VkRenderPass m_renderPass = VK_NULL_HANDLE;
        CommandPool* m_commandPool = nullptr;
        std::vector<VkCommandBuffer> m_graphicsCommandBuffers;
        std::vector<VkCommandBuffer> m_computeCommandBuffers;
        // Graphics Pipeline
        VkDescriptorSetLayout m_graphicsDescriptorSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout m_graphicsPipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
        VkPipeline m_particleGraphicsPipeline = VK_NULL_HANDLE;
        // Compute Pipeline
        VkDescriptorSetLayout m_computeDescriptorSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout m_computePipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_computePipeline = VK_NULL_HANDLE;

        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_graphicsDescriptorSets;
        std::vector<VkDescriptorSet> m_computeDescriptorSets;

        uint m_currentFrame = 0;
        uint m_imageIndex = 0;
        uint m_accumulatedFrameNum = 0;
        uint m_frameNum = 0;

        inline static VulkanBackendApp* s_application = nullptr;

        std::vector<UniformBuffer*> m_MVPUniformBuffers;

        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence> m_graphicsInFlightFences;
        std::vector<VkFence> m_computeInFlightFences;
        std::vector<VkSemaphore> m_computeFinishedSemaphores;

        ImGuiInfrastructure* m_imguiInfrastructure = nullptr;
        glm::vec2 m_viewportSize = glm::vec2(0.f, 0.f);

        uint m_msaaSamples = 8;

        MSAABuffer* m_msaaBuffers = nullptr;

        // For GPU Particles
        inline static uint s_particleCount = 81920;
        std::vector<StorageBuffer*> m_particleStorageBuffers;
        void CreateParticleStorageBuffers();
        std::shared_ptr<VertexBufferLayout> m_particleVertexBufferLayout;

        std::shared_ptr<CameraBase> m_camera;

        Debugger* m_debugger = nullptr;  // TODO
    };
} // namespace Shadowy

#endif //HARDWAREPATHTRACER_VULKANBACKENDAPP_H
