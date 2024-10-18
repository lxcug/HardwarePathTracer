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
#include "core/Model.h"
#include "core/renderGraph/RasterPass.h"


namespace HWPT {
    const int MAX_FRAMES_IN_FLIGHT = 2;

    struct QueueFamilyIndices {
        std::optional<uint> GraphicsFamily;
        std::optional<uint> ComputeFamily;
        std::optional<uint> PresentFamily;

        [[nodiscard]] auto IsComplete() const -> bool {
            return GraphicsFamily.has_value() && ComputeFamily.has_value() && PresentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR Capabilities;
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    struct Queue {
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

    struct MSAABuffer {
        Texture2D* MSAAColorBuffer = nullptr;
        Texture2D* MSAADepthBuffer = nullptr;

        ~MSAABuffer() {
            delete MSAAColorBuffer;
            delete MSAADepthBuffer;
        }
    };

    class VulkanBackendApp : public ApplicationBase {
    public:
        void Run() override;

        void Init() override;

        void DrawFrame() override;

        void DrawImGuiFrame();

        auto BeginIntermediateCommand() -> VkCommandBuffer;

        void EndIntermediateCommand(VkCommandBuffer commandBuffer);

        auto GetVkInstance() -> VkInstance {
            return m_instance;
        }

        auto GetVkDevice() -> VkDevice {
            return m_device;
        }

        auto GetPhysicalDevice() -> VkPhysicalDevice {
            return m_physicalDevice;
        }

        static auto GetApplication() -> VulkanBackendApp* {
            return s_application;
        }

        auto GetSwapChain() -> SwapChain {
            return m_swapChain;
        }

        [[nodiscard]] auto GetMSAASampleCount() const -> uint {
            return m_msaaSamples;
        }

    private:
        // Init GLFW Windows
        void InitWindow();

        // Init Vulkan Backend
        void InitVulkan();

        void CleanUp();

        void Present();

        // FrameBuffer Resize Callback
        static void FrameBufferResizeCallback(GLFWwindow* Window, int Width, int Height);

        void RecreateSwapChain();

        void CreateMSAABuffers();

    private:
        void InitImGui();

        void BeginImGui();

        void EndImGui();

        void CleanUpImGui();

        void EnableWholeScreenDocking();

    private:
        // VulkanContext Init
        void CreateVkInstance();

        static auto GetRequiredExtensions() -> std::vector<const char*>;

        void CreateSurface();

        void SelectPhysicalDevice();

        auto FindQueueFamilies(VkPhysicalDevice PhysicalDevice) -> QueueFamilyIndices;

        static auto IsDeviceExtensionSupport(VkPhysicalDevice PhysicalDevice) -> bool;

        auto QuerySwapChainSupport(VkPhysicalDevice PhysicalDevice) -> SwapChainSupportDetails;

        auto IsSuitableDevice(VkPhysicalDevice PhysicalDevice) -> bool;

        void CreateLogicalDevice();

        void CreateSwapChain();

        void CleanUpSwapChain();

        void CreateFrameBuffers();

        void CreateCommandPool();

        void CreateCommandBuffers();

        void CreateComputeDescriptorSetLayout();

        void CreateComputePipeline();

        void CreateDescriptorPool();

        void CreateGraphicsDescriptorSets();

        void CreateComputeDescriptorSets();

        void CreateSyncObjects();

        void RecordCommandBuffer(VkCommandBuffer CommandBuffer, uint ImageIndex);

        void CreateUniformBuffers();

        void CreateModelAndSampler();

        void OnWindowResize();

        void CreateRasterPass();

    protected:
        VkDevice m_device = VK_NULL_HANDLE;

        std::string m_windowTitle = "VulkanBackend Application";
        uint m_windowWidth = 1600, m_windowHeight = 900;
        GLFWwindow* m_window = nullptr;
        bool m_frameBufferResized = false;
        bool m_contextInited = false;

        std::shared_ptr<FPSCalculator> m_fpsCalculator;

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
                VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
                VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
                VK_KHR_RAY_QUERY_EXTENSION_NAME,
                VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
        };

        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        Queue m_queue;
        SwapChain m_swapChain;
        std::vector<VkFramebuffer> m_swapChainFrameBuffers;
//        SwapChain m_viewportSwapChain;  // TODO
//        std::vector<VkFramebuffer> m_viewportFrameBuffer;
        CommandPool m_commandPool;
        std::vector<VkCommandBuffer> m_graphicsCommandBuffers;
        std::vector<VkCommandBuffer> m_computeCommandBuffers;

        // Graphics Pipeline
//        VkDescriptorSetLayout m_graphicsDescriptorSetLayout = VK_NULL_HANDLE;
//        VkPipelineLayout m_graphicsPipelineLayout = VK_NULL_HANDLE;
//        VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

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

        inline static VulkanBackendApp* s_application = nullptr;

        std::vector<UniformBuffer*> m_MVPUniformBuffers;
        Sampler* m_sampler = nullptr;

        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence> m_graphicsInFlightFences;
        std::vector<VkFence> m_computeInFlightFences;
        std::vector<VkSemaphore> m_computeFinishedSemaphores;

        ImGuiInfrastructure* m_imguiInfrastructure = nullptr;

        glm::vec2 m_viewportSize = glm::vec2(0.f, 0.f);

        Model* m_vikingRoom = nullptr;
        uint m_msaaSamples = 8;

        MSAABuffer* m_msaaBuffers = nullptr;

        // For GPU Particles
        inline static uint s_particleCount = 81920;
        std::vector<StorageBuffer*> m_particleStorageBuffers;
        void CreateParticleStorageBuffers();
        std::shared_ptr<VertexBufferLayout> m_particleVertexBufferLayout;

        RasterPass* m_rasterPass = nullptr;
        RasterPass* m_particlePass = nullptr;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_VULKANBACKENDAPP_H
