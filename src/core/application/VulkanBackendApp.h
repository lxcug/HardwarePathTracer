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
#include "core/renderGraph/ComputePass.h"
#include "core/buffer/FrameBuffer.h"
#include "core/SwapChain.h"
#include "core/renderGraph/RenderGraph.h"


namespace HWPT {
    const int MAX_FRAMES_IN_FLIGHT = 2;

    struct Queue {
        VkQueue GraphicsQueue = VK_NULL_HANDLE;
        VkQueue ComputeQueue = VK_NULL_HANDLE;
        VkQueue PresentQueue = VK_NULL_HANDLE;
    };

    struct CommandPool {
        VkCommandPool GraphicsPool = VK_NULL_HANDLE;
        VkCommandPool ComputePool = VK_NULL_HANDLE;
    };

    struct MSAABuffer {
        Texture2D *MSAAColorBuffer = nullptr;
        Texture2D *MSAADepthBuffer = nullptr;

        ~MSAABuffer() {
            delete MSAAColorBuffer;
            delete MSAADepthBuffer;
        }
    };

    struct MVPData {
        glm::mat4 ModelTrans;
        glm::mat4 ViewTrans;
        glm::mat4 ProjTrans;
        glm::vec3 DebugColor;
        float DeltaTime;
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

        static auto GetApplication() -> VulkanBackendApp * {
            return s_application;
        }

        auto GetSwapChain() -> SwapChain * {
            return m_swapChain;
        }

        [[nodiscard]] auto GetSwapChain() const -> const SwapChain * {
            return m_swapChain;
        }

        [[nodiscard]] auto GetMSAASampleCount() const -> uint {
            return m_msaaSamples;
        }

        auto GetGlobalSampler() -> Sampler * {
            return m_sampler;
        }

        auto GetGlobalDescriptorPool() -> VkDescriptorPool & {
            return m_descriptorPool;
        }

        auto GetGlobalQueue() -> Queue& {
            return m_queue;
        }

        auto GetCurrentFrameBuffer() -> FrameBuffer* {
            return m_basePassFrameBuffers[m_currentFrame];
        }

    private:
        // Init GLFW Windows
        void InitWindow();

        // Init Vulkan Backend
        void InitVulkan();

        void InitVulkanInfrastructure();

        void CleanUp();

        void Present();

        // FrameBuffer Resize Callback
        static void FrameBufferResizeCallback(GLFWwindow *Window, int Width, int Height);

        void RecreateSwapChain();

        void CreateMSAABuffers();

        void UpdateViewUniformBuffer();

    private:
        void InitImGui();

        void BeginImGui();

        void EndImGui();

        void CleanUpImGui();

        void EnableWholeScreenDocking();

    private:
        // VulkanContext Init
        void CreateVkInstance();

        static auto GetRequiredExtensions() -> std::vector<const char *>;

        void CreateSurface();

        void SelectPhysicalDevice();

        static auto IsDeviceExtensionSupport(VkPhysicalDevice PhysicalDevice) -> bool;

        auto IsSuitableDevice(VkPhysicalDevice PhysicalDevice) -> bool;

        void CreateLogicalDevice();

        void CreateSwapChain();

        void CreateFrameBuffers();

        void CreateCommandPool();

        void CreateCommandBuffers();

        void CreateDescriptorPool();

        void CreateSyncObjects();

        void CreateUniformBuffers();

        void CreateModelAndSampler();

        void CreateParticleStorageBuffers();

        void OnWindowResize();

        void CreateRasterPass();

        void CreateComputePass();

    protected:
        std::string m_windowTitle = "VulkanBackend Application";
        uint m_windowWidth = 1600, m_windowHeight = 900;
        GLFWwindow *m_window = nullptr;
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

        VkDevice m_device = VK_NULL_HANDLE;
        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

        SwapChain *m_swapChain = nullptr;
        uint m_msaaSamples = 8;
        MSAABuffer *m_msaaBuffers = nullptr;
        std::vector<FrameBuffer *> m_basePassFrameBuffers;

        Queue m_queue;
        CommandPool m_commandPool;
        std::vector<VkCommandBuffer> m_graphicsCommandBuffers;
        std::vector<VkCommandBuffer> m_computeCommandBuffers;

        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

        ImGuiInfrastructure *m_imguiInfrastructure = nullptr;

//        SwapChain m_viewportSwapChain;  // TODO
//        std::vector<VkFramebuffer> m_viewportFrameBuffer;

        uint m_currentFrame = 0;
        uint m_imageIndex = 0;

        inline static VulkanBackendApp *s_application = nullptr;

        std::vector<UniformBuffer *> m_MVPUniformBuffers;
        Sampler *m_sampler = nullptr;

        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence> m_graphicsInFlightFences;
        std::vector<VkFence> m_computeInFlightFences;
        std::vector<VkSemaphore> m_computeFinishedSemaphores;

        glm::vec2 m_viewportSize = glm::vec2(0.f, 0.f);

        Model *m_vikingRoom = nullptr;

        // For GPU Particles
        inline static uint s_particleCount = 81920;
        std::vector<StorageBuffer *> m_particleStorageBuffers;
        std::shared_ptr<VertexBufferLayout> m_particleVertexBufferLayout;

        RasterPass *m_rasterPass = nullptr;
        RasterPass *m_particlePass = nullptr;
        ComputePass *m_updateParticlePass = nullptr;

        RenderGraph* m_renderGraph = nullptr;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_VULKANBACKENDAPP_H
