//
// Created by HUSTLX on 2024/10/7.
//

#include "VulkanBackendApp.h"
#include <unordered_set>
#include <algorithm>
#include <core/shader/ShaderBase.h>
#include <core/buffer/VertexBuffer.h>
#include "core/RHI.h"
#include <random>
#include "core/shader_compiler/HLSLCompiler.h"
#include "core/Utils.h"


namespace Shadowy {
    auto GetVKDevice() -> VkDevice {
        return VulkanBackendApp::GetApplication()->GetVkDevice();
    }

    auto GetVKPhysicalDevice() -> VkPhysicalDevice {
        return VulkanBackendApp::GetApplication()->GetPhysicalDevice();
    }

    auto GetViewUniformBuffers() -> const std::vector<UniformBuffer*>& {
        return VulkanBackendApp::GetApplication()->GetViewUniformBuffers();
    }

    void VulkanBackendApp::Run() {
        Check(m_contextInited);

        while (!glfwWindowShouldClose(m_window)) {
            m_fpsCalculator->Tick();

            if (m_camera->IsMoving()) {
                ResetFrameNum();
            }

            glfwPollEvents();
            RenderFrame();

            Present();
            m_accumulatedFrameNum++;
            m_frameNum++;
            m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        }

        vkDeviceWaitIdle(m_device);
        CleanUp();
    }

    void VulkanBackendApp::DrawImGuiFrame() {
        m_camera->Tick(m_fpsCalculator->GetDeltaTime());

        {
            bool Open;
            ImGuiInfrastructure::Begin("Settings", &Open, ImGuiDockNodeFlags_AutoHideTabBar);

            ImGuiInfrastructure::End();
        }
        {
            ImGuiInfrastructure::Begin("Statistics");
            ImGui::Text("FPS: %d", m_fpsCalculator->GetFPS());
            ImGuiInfrastructure::End();
        }
    }

    void VulkanBackendApp::Init() {
        Check(s_application == nullptr);
        s_application = this;
        InitWindow();
        InitVulkan();
        m_contextInited = true;
        m_debugger = new Debugger();
        m_fpsCalculator = std::make_shared<FPSCalculator>(1.f);
    }

    void VulkanBackendApp::InitWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        m_window = glfwCreateWindow(
                static_cast<int>(m_windowWidth), static_cast<int>(m_windowHeight),
                m_windowTitle.c_str(), nullptr, nullptr);

        GLFWmonitor *PrimaryMonitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *Mode = glfwGetVideoMode(PrimaryMonitor);
        int PosX = static_cast<int>((Mode->width - m_windowWidth) / 2);
        int PosY = static_cast<int>((Mode->height - m_windowHeight) / 2);

        glfwSetWindowPos(m_window, PosX, PosY);

        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, FrameBufferResizeCallback);
//        glfwSetScrollCallback(m_window, MouseScrollCallBack);
    }

    void VulkanBackendApp::InitVulkan() {
        CreateVkInstance();
        CreateSurface();
        SelectPhysicalDevice();
        CreateLogicalDevice();
        CreateCommandPool();
        CreateCommandBuffers();
        CreateSwapChain();

        InitImGui();

        CreateRenderPass();
        CreateMSAABuffers();
        CreateFrameBuffers();

        CreateDescriptorPool();

        CreateViewUniformBuffers();

        CreateGraphicsDescriptorSetLayout();
        CreateGraphicsPipeline();
        CreateParticleGraphicsPipeline();
        CreateGraphicsDescriptorSets();

        CreateParticleStorageBuffers();
        CreateComputeDescriptorSetLayout();
        CreateComputePipeline();
        CreateComputeDescriptorSets();

        CreateSyncObjects();
    }

    void VulkanBackendApp::FrameBufferResizeCallback(GLFWwindow *Window, int Width, int Height) {
        auto App =
                reinterpret_cast<VulkanBackendApp *>(glfwGetWindowUserPointer(Window));
        App->m_frameBufferResized = true;
    }

    void VulkanBackendApp::MouseScrollCallBack(GLFWwindow *Window, double XOffset, double YOffset) {
        auto App =
                reinterpret_cast<VulkanBackendApp *>(glfwGetWindowUserPointer(Window));
        App->GetCamera()->OnMouseScroll(XOffset, YOffset);
    }

    void VulkanBackendApp::CleanUp() {
        Sampler::ReleaseSamplers();
        delete m_msaaBuffers;
//        delete m_vikingRoom;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            delete m_ViewUniformBuffers[i];
            delete m_particleStorageBuffers[i];
        }

        CleanUpImGui();
        CleanUpSwapChain();

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(m_device, m_frameInFlightFences[i], nullptr);
            vkDestroyFence(m_device, m_computeInFlightFences[i], nullptr);
            vkDestroySemaphore(m_device, m_computeFinishedSemaphores[i], nullptr);
        }

        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_graphicsDescriptorSetLayout, nullptr);
        vkDestroyPipelineLayout(m_device, m_graphicsPipelineLayout, nullptr);
        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        vkDestroyPipeline(m_device, m_particleGraphicsPipeline, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_computeDescriptorSetLayout, nullptr);
        vkDestroyPipelineLayout(m_device, m_computePipelineLayout, nullptr);
        vkDestroyPipeline(m_device, m_computePipeline, nullptr);
        delete m_commandPool;
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyDevice(m_device, nullptr);
        vkDestroyInstance(m_instance, nullptr);

        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    void VulkanBackendApp::RenderFrame() {
        vkWaitForFences(m_device, 1, &m_computeInFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(m_device, 1, &m_computeInFlightFences[m_currentFrame]);

        auto ComputeCommandBuffer = m_computeCommandBuffers[m_currentFrame];
        vkResetCommandBuffer(ComputeCommandBuffer, 0);

        VkCommandBufferBeginInfo ComputeBeginInfo{};
        ComputeBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_CHECK(vkBeginCommandBuffer(ComputeCommandBuffer, &ComputeBeginInfo));
        vkCmdBindPipeline(ComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);
        vkCmdBindDescriptorSets(ComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                m_computePipelineLayout, 0, 1,
                                &m_computeDescriptorSets[m_currentFrame], 0, nullptr);
        vkCmdDispatch(ComputeCommandBuffer, (s_particleCount + 255) / 256, 1, 1);
        VK_CHECK(vkEndCommandBuffer(ComputeCommandBuffer));

        VkSubmitInfo ComputeSubmitInfo{};
        ComputeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        ComputeSubmitInfo.waitSemaphoreCount = 0;
        ComputeSubmitInfo.commandBufferCount = 1;
        ComputeSubmitInfo.pCommandBuffers = &ComputeCommandBuffer;
        ComputeSubmitInfo.signalSemaphoreCount = 1;
        ComputeSubmitInfo.pSignalSemaphores = &m_computeFinishedSemaphores[m_currentFrame];
        VK_CHECK(vkQueueSubmit(m_queue.ComputeQueue, 1, &ComputeSubmitInfo,
                               m_computeInFlightFences[m_currentFrame]));


        vkWaitForFences(m_device, 1, &m_frameInFlightFences[m_currentFrame], VK_TRUE,
                        UINT64_MAX);
        vkResetFences(m_device, 1, &m_frameInFlightFences[m_currentFrame]);

        if (m_frameBufferResized) {
            OnWindowResize();
            m_frameBufferResized = false;
        }

        VkResult Result = vkAcquireNextImageKHR(m_device, m_swapChain.SwapChainHandle, UINT64_MAX,
                                                m_imageAvailableSemaphores[m_currentFrame],
                                                VK_NULL_HANDLE, &m_imageIndex);
        if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR) {
            OnWindowResize();
        } else if (Result != VK_SUCCESS) {
            throw std::runtime_error("Failed to acquire swap chain images");
        }

        auto GraphicsCommandBuffer = m_graphicsCommandBuffers[m_currentFrame];
        vkResetCommandBuffer(GraphicsCommandBuffer, 0);

        RecordCommandBuffer(GraphicsCommandBuffer, m_imageIndex);

        // TODO: Use the same command buffer for better performance
        m_imguiInfrastructure->BeginImGui();
        DrawImGuiFrame();
        m_imguiInfrastructure->EndImGui(GraphicsCommandBuffer);
        VK_CHECK(vkEndCommandBuffer(GraphicsCommandBuffer));

        VkSubmitInfo GraphicsSubmitInfo{};
        GraphicsSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        std::array<VkSemaphore, 2> GraphicsWaitSemaphores = {
                m_computeFinishedSemaphores[m_currentFrame],
                m_imageAvailableSemaphores[m_currentFrame]
        };
        GraphicsSubmitInfo.waitSemaphoreCount = GraphicsWaitSemaphores.size();
        GraphicsSubmitInfo.pWaitSemaphores = GraphicsWaitSemaphores.data();
        std::array<VkPipelineStageFlags, 2> WaitStages = {
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
        };
        GraphicsSubmitInfo.pWaitDstStageMask = WaitStages.data();
        GraphicsSubmitInfo.commandBufferCount = 1;
        GraphicsSubmitInfo.pCommandBuffers = &GraphicsCommandBuffer;
        GraphicsSubmitInfo.signalSemaphoreCount = 1;
        GraphicsSubmitInfo.pSignalSemaphores = &m_renderFinishedSemaphores[m_currentFrame];
        VK_CHECK(vkQueueSubmit(m_queue.GraphicsQueue, 1, &GraphicsSubmitInfo,
                               m_frameInFlightFences[m_currentFrame]));
    }

    void VulkanBackendApp::CreateVkInstance() {
        VkApplicationInfo AppInfo{};
        AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        AppInfo.pApplicationName = m_windowTitle.c_str();
        AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        AppInfo.pEngineName = "No Engine";
        AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        AppInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        auto Extensions = GetRequiredExtensions();
        CreateInfo.pApplicationInfo = &AppInfo;
        CreateInfo.enabledExtensionCount = Extensions.size();
        CreateInfo.ppEnabledExtensionNames = Extensions.data();
        if (m_enableValidationLayers) {
            CreateInfo.enabledLayerCount = ValidationLayers.size();
            CreateInfo.ppEnabledLayerNames = ValidationLayers.data();
        }

        VK_CHECK(vkCreateInstance(&CreateInfo, nullptr, &m_instance));
    }

    auto VulkanBackendApp::GetRequiredExtensions() -> std::vector<const char *> {
        uint glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char *> Extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#if !BUILD_RELEASE && !BUILD_SHIPPING
        Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        //        std::cout << "Enabled Extensions:\n";
        //        for (const auto &ExtensionName: Extensions) {
        //            std::cout << "\t" << ExtensionName << std::endl;
        //        }
        //        std::cout.flush();

        return Extensions;
    }

    void VulkanBackendApp::CreateSurface() {
        VK_CHECK(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface));
    }

    void VulkanBackendApp::SelectPhysicalDevice() {
        uint DeviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &DeviceCount, nullptr);
        std::vector<VkPhysicalDevice> Devices(DeviceCount);
        vkEnumeratePhysicalDevices(m_instance, &DeviceCount, Devices.data());

        for (auto Device: Devices) {
            if (IsSuitableDevice(Device)) {
                m_physicalDevice = Device;
                break;
            }
        }

        Check(m_physicalDevice != VK_NULL_HANDLE);
        VkPhysicalDeviceProperties DeviceProperty;
        vkGetPhysicalDeviceProperties(m_physicalDevice, &DeviceProperty);
        std::cout << "Device Name: " << DeviceProperty.deviceName << "\n";
        std::cout.flush();
    }

    auto VulkanBackendApp::IsDeviceExtensionSupport(VkPhysicalDevice PhysicalDevice) -> bool {
        uint ExtensionCount = 0;
        vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &ExtensionCount, nullptr);
        std::vector<VkExtensionProperties> AvailableExtensions(ExtensionCount);
        vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr,
                                             &ExtensionCount, AvailableExtensions.data());

        std::unordered_set<std::string> RequiredExtensionsCopy(DeviceExtensions.begin(),
                                                               DeviceExtensions.end());
        for (const auto &SupportExtension: AvailableExtensions) {
            RequiredExtensionsCopy.erase(SupportExtension.extensionName);
        }

        return RequiredExtensionsCopy.empty();
    }

    auto
    VulkanBackendApp::QuerySwapChainSupport(
            VkPhysicalDevice PhysicalDevice) -> SwapChainSupportDetails {
        SwapChainSupportDetails Details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, m_surface, &Details.Capabilities);

        uint FormatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, m_surface, &FormatCount, nullptr);
        if (FormatCount > 0) {
            Details.Formats.resize(FormatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, m_surface,
                                                 &FormatCount, Details.Formats.data());
        }

        uint PresentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, m_surface, &PresentModeCount,
                                                  nullptr);

        if (PresentModeCount > 0) {
            Details.PresentModes.resize(PresentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, m_surface,
                                                      &PresentModeCount,
                                                      Details.PresentModes.data());
        }

        return Details;
    }

    auto VulkanBackendApp::IsSuitableDevice(VkPhysicalDevice PhysicalDevice) -> bool {
        QueueFamilyIndices Indices = Utils::FindQueueFamilies(PhysicalDevice);
        bool IsExtensionSupport = IsDeviceExtensionSupport(PhysicalDevice);
        auto SwapChainSupport = QuerySwapChainSupport(PhysicalDevice);
        bool IsSwapChainSupport =
                !SwapChainSupport.Formats.empty() && !SwapChainSupport.PresentModes.empty();

        return Indices.IsComplete() && IsExtensionSupport && IsSwapChainSupport;
    }

    void VulkanBackendApp::CreateLogicalDevice() {
        QueueFamilyIndices Indices = Utils::FindQueueFamilies();

        std::unordered_set<uint> UniqueQueueFamilies = {
                Indices.GraphicsFamily.value(),
                Indices.ComputeFamily.value(),
                Indices.PresentFamily.value()
        };
        std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
        QueueCreateInfos.reserve(QueueCreateInfos.size());

        float QueuePriority = 1.f;
        for (uint QueueFamily: UniqueQueueFamilies) {
            VkDeviceQueueCreateInfo QueueCreateInfo{};
            QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            QueueCreateInfo.queueFamilyIndex = QueueFamily;
            QueueCreateInfo.queueCount = 1;
            QueueCreateInfo.pQueuePriorities = &QueuePriority;
            QueueCreateInfos.push_back(QueueCreateInfo);
        }

        VkDeviceCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        CreateInfo.queueCreateInfoCount = 1;
        CreateInfo.pQueueCreateInfos = QueueCreateInfos.data();
        VkPhysicalDeviceFeatures DeviceFeatures{};
        DeviceFeatures.sampleRateShading = VK_TRUE;
        DeviceFeatures.geometryShader = VK_TRUE;
        DeviceFeatures.shaderInt64 = VK_TRUE;
        CreateInfo.pEnabledFeatures = &DeviceFeatures;
        CreateInfo.enabledExtensionCount = DeviceExtensions.size();
        CreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

        if (m_enableValidationLayers) {
            CreateInfo.enabledLayerCount = ValidationLayers.size();
            CreateInfo.ppEnabledLayerNames = ValidationLayers.data();
        }

        // Sync2
        VkPhysicalDeviceSynchronization2FeaturesKHR Sync2Feature{
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR
        };
        Sync2Feature.synchronization2 = VK_TRUE;
        CreateInfo.pNext = &Sync2Feature;
        // RayTracingPipeline
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR RayTracingPipelineFeatures{
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR
        };
        RayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
        // AccelerationStructure
        VkPhysicalDeviceAccelerationStructureFeaturesKHR ASFeature{
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR
        };
        ASFeature.accelerationStructure = VK_TRUE;
        // Ray Query
        VkPhysicalDeviceRayQueryFeaturesKHR RayQueryFeatures{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR
        };
        RayQueryFeatures.rayQuery = VK_TRUE;
        // RunTimeDescriptorArray(bindless) and BufferDeviceAddress
        VkPhysicalDeviceVulkan12Features Vulkan12Features = {
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
        };
        Vulkan12Features.runtimeDescriptorArray = VK_TRUE;
        Vulkan12Features.bufferDeviceAddress = VK_TRUE;

        // Link Features
        RayQueryFeatures.pNext = &Vulkan12Features;
        ASFeature.pNext = &RayQueryFeatures;
        Sync2Feature.pNext = &ASFeature;
        RayTracingPipelineFeatures.pNext = &Sync2Feature;
        CreateInfo.pNext = &RayTracingPipelineFeatures;

        VK_CHECK(vkCreateDevice(m_physicalDevice, &CreateInfo, nullptr, &m_device));
    }

    void VulkanBackendApp::CreateSwapChain() {
        SwapChainSupportDetails SwapChainSupport = QuerySwapChainSupport(m_physicalDevice);

        VkSurfaceFormatKHR SurfaceFormat;
        for (auto Format: SwapChainSupport.Formats) {
            if (Format.format == VK_FORMAT_R8G8B8A8_UNORM) {
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
        VkExtent2D Extent;
        if (SwapChainSupport.Capabilities.currentExtent.width != UINT_MAX) {
            Extent = SwapChainSupport.Capabilities.currentExtent;
        } else {
            int Width, Height;
            glfwGetFramebufferSize(m_window, &Width, &Height);
            Extent = {static_cast<uint>(Width), static_cast<uint>(Height)};
            VkExtent2D &MinExtent = SwapChainSupport.Capabilities.minImageExtent;
            VkExtent2D &MaxExtent = SwapChainSupport.Capabilities.maxImageExtent;
            Extent.width = std::clamp(Extent.width, MinExtent.width, MaxExtent.width);
            Extent.height = std::clamp(Extent.height, MinExtent.height, MaxExtent.height);
        }

        uint ImageCount = std::clamp(SwapChainSupport.Capabilities.minImageCount,
                                     SwapChainSupport.Capabilities.minImageCount,
                                     SwapChainSupport.Capabilities.maxImageCount);
        Check(MAX_FRAMES_IN_FLIGHT >= ImageCount);

        VkSwapchainCreateInfoKHR CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        CreateInfo.surface = m_surface;
        CreateInfo.minImageCount = ImageCount;
        CreateInfo.imageFormat = SurfaceFormat.format;
        CreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
        CreateInfo.presentMode = PresentMode;
        CreateInfo.imageExtent = Extent;
        CreateInfo.imageArrayLayers = 1;
        CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                VK_IMAGE_USAGE_STORAGE_BIT;

        QueueFamilyIndices Indices = m_commandPool->GetQueueFamilyIndices();
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

        VK_CHECK(
                vkCreateSwapchainKHR(m_device, &CreateInfo, nullptr, &m_swapChain.SwapChainHandle));
        m_swapChain.Extent = Extent;
        m_swapChain.Format = SurfaceFormat.format;
        m_swapChain.GetImages(m_device);
        m_swapChain.CreateImageViews(m_device);
    }

    void VulkanBackendApp::CleanUpSwapChain() {
        for (auto &FrameBuffer: m_swapChainFrameBuffers) {
            vkDestroyFramebuffer(m_device, FrameBuffer, nullptr);
        }
        for (auto &ImageView: m_swapChain.SwapChainImageViews) {
            vkDestroyImageView(m_device, ImageView, nullptr);
        }
        vkDestroySwapchainKHR(m_device, m_swapChain.SwapChainHandle, nullptr);
    }

    void VulkanBackendApp::CreateRenderPass() {
        VkAttachmentDescription MSAAColorAttachment{};
        MSAAColorAttachment.format = m_swapChain.Format;
        MSAAColorAttachment.samples = GetVKSampleCount(m_msaaSamples);
        MSAAColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        MSAAColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        MSAAColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        MSAAColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        MSAAColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        MSAAColorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        // For resolve MSAA buffer

        VkAttachmentDescription DepthAttachment{};
        DepthAttachment.format = GetVKFormat(TextureFormat::Depth32);
        DepthAttachment.samples = GetVKSampleCount(m_msaaSamples);
        DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription ResolvedColorAttachment{};
        ResolvedColorAttachment.format = m_swapChain.Format;
        ResolvedColorAttachment.samples = GetVKSampleCount(1u);
        ResolvedColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ResolvedColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        ResolvedColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ResolvedColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        ResolvedColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ResolvedColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference MSAAColorAttachmentRef{};
        MSAAColorAttachmentRef.attachment = 0;
        MSAAColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference DepthAttachmentRef{};
        DepthAttachmentRef.attachment = 1;
        DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference ResolvedColorAttachmentRef{};
        ResolvedColorAttachmentRef.attachment = 2;
        ResolvedColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription SubPass{};
        SubPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        SubPass.colorAttachmentCount = 1;
        SubPass.pColorAttachments = &MSAAColorAttachmentRef;
        SubPass.pDepthStencilAttachment = &DepthAttachmentRef;
        SubPass.pResolveAttachments = &ResolvedColorAttachmentRef;

        VkSubpassDependency Dependency{};
        Dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        Dependency.dstSubpass = 0;
        Dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        Dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        Dependency.srcAccessMask = 0;
        Dependency.dstAccessMask =
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 3> Attachments = {
                MSAAColorAttachment,
                DepthAttachment,
                ResolvedColorAttachment
        };
        VkRenderPassCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        CreateInfo.attachmentCount = Attachments.size();
        CreateInfo.pAttachments = Attachments.data();
        CreateInfo.subpassCount = 1;
        CreateInfo.pSubpasses = &SubPass;
        CreateInfo.dependencyCount = 1;
        CreateInfo.pDependencies = &Dependency;

        VK_CHECK(vkCreateRenderPass(m_device, &CreateInfo, nullptr, &m_renderPass));
    }

    void VulkanBackendApp::CreateFrameBuffers() {
        m_swapChainFrameBuffers.resize(m_swapChain.SwapChainImages.size());

        for (size_t Index = 0; Index < m_swapChainFrameBuffers.size(); Index++) {
            std::array<VkImageView, 3> Attachments = {
                    m_msaaBuffers->MSAAColorBuffer->CreateSRV(),
                    m_msaaBuffers->MSAADepthBuffer->CreateSRV(),
                    m_swapChain.SwapChainImageViews[Index]
            };

            VkFramebufferCreateInfo CreateInfo{};
            CreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            CreateInfo.renderPass = m_renderPass;
            CreateInfo.width = m_swapChain.Extent.width;
            CreateInfo.height = m_swapChain.Extent.height;
            CreateInfo.layers = 1;
            CreateInfo.attachmentCount = Attachments.size();
            CreateInfo.pAttachments = Attachments.data();

            VK_CHECK(vkCreateFramebuffer(m_device, &CreateInfo, nullptr,
                                         &m_swapChainFrameBuffers[Index]));
        }
    }

    void VulkanBackendApp::CreateCommandPool() {
        m_commandPool = new CommandPool(PoolType::Graphics | PoolType::Compute);
        m_queue.GraphicsQueue = m_commandPool->GetGraphicsQueue();
        m_queue.ComputeQueue = m_commandPool->GetComputeQueue();
        m_queue.PresentQueue = m_commandPool->GetPresentQueue();
    }

    void VulkanBackendApp::CreateCommandBuffers() {
        m_graphicsCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        m_computeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo GraphicsAllocateInfo{};
        GraphicsAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        GraphicsAllocateInfo.commandPool = m_commandPool->GetGraphicsPool();
        GraphicsAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        GraphicsAllocateInfo.commandBufferCount = m_graphicsCommandBuffers.size();
        VK_CHECK(vkAllocateCommandBuffers(m_device, &GraphicsAllocateInfo,
                                          m_graphicsCommandBuffers.data()));

        VkCommandBufferAllocateInfo ComputeAllocateInfo{};
        ComputeAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ComputeAllocateInfo.commandPool = m_commandPool->GetComputePool();
        ComputeAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ComputeAllocateInfo.commandBufferCount = m_computeCommandBuffers.size();

        VK_CHECK(vkAllocateCommandBuffers(m_device, &ComputeAllocateInfo,
                                          m_computeCommandBuffers.data()));
    }

    void VulkanBackendApp::CreateGraphicsDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding UBOLayoutBinding{};
        UBOLayoutBinding.binding = 0;
        UBOLayoutBinding.descriptorCount = 1;
        UBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        UBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        UBOLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding SamplerLayoutBinding{};
        SamplerLayoutBinding.binding = 1;
        SamplerLayoutBinding.descriptorCount = 1;
        SamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        SamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> Bindings = {
                UBOLayoutBinding, SamplerLayoutBinding
        };

        VkDescriptorSetLayoutCreateInfo LayoutInfo{};
        LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        LayoutInfo.bindingCount = Bindings.size();
        LayoutInfo.pBindings = Bindings.data();

        VK_CHECK(vkCreateDescriptorSetLayout(m_device, &LayoutInfo, nullptr,
                                             &m_graphicsDescriptorSetLayout));
    }

    void VulkanBackendApp::CreateGraphicsPipeline() {
        HLSLCompiler::CompileShader("Mesh.hlsl", "VSMain", Shadowy::ShaderType::Vertex, "Vert");
        HLSLCompiler::CompileShader("Mesh.hlsl", "PSMain", Shadowy::ShaderType::Fragment, "Frag");
        HLSLCompiler::CompileShader("Particle.hlsl", "VSMain", Shadowy::ShaderType::Vertex,
                                    "ParticleVert");
        HLSLCompiler::CompileShader("Particle.hlsl", "PSMain", Shadowy::ShaderType::Fragment,
                                    "ParticleFrag");
        HLSLCompiler::CompileShader("UpdateParticle.hlsl", "UpdateParticles",
                                    Shadowy::ShaderType::Compute, "UpdateParticle");

        ShaderBase VertexShader(ShaderType::Vertex, "../../shader/HLSL/Vert.spv", "VSMain");
        ShaderBase FragmentShader(ShaderType::Fragment, "../../shader/HLSL/Frag.spv", "PSMain");

        VkPipelineShaderStageCreateInfo VertShaderStageInfo{};
        VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        VertShaderStageInfo.module = VertexShader.GetHandle();
        VertShaderStageInfo.pName = VertexShader.GetEntryName();
        VkPipelineShaderStageCreateInfo FragShaderStageInfo{};
        FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        FragShaderStageInfo.module = FragmentShader.GetHandle();
        FragShaderStageInfo.pName = FragmentShader.GetEntryName();

        std::array<VkPipelineShaderStageCreateInfo, 2> ShaderStages = {
                VertShaderStageInfo, FragShaderStageInfo
        };

        VkPipelineVertexInputStateCreateInfo VertexInputInfo{};
        VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        auto bindingDescription = VertexBuffer::DefaultVertexLayout().GetBindingDescription();
        auto attributeDescription = VertexBuffer::DefaultVertexLayout().GetAttributeDescriptions();
        VertexInputInfo.vertexBindingDescriptionCount = 1;
        VertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        VertexInputInfo.vertexAttributeDescriptionCount = attributeDescription.size();
        VertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

        VkPipelineInputAssemblyStateCreateInfo InputAssemble{};
        InputAssemble.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        InputAssemble.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        InputAssemble.primitiveRestartEnable = VK_FALSE;

        std::vector<VkDynamicState> DynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,
                VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT
        };
        VkPipelineDynamicStateCreateInfo DynamicState{};
        DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        DynamicState.dynamicStateCount = DynamicStates.size();
        DynamicState.pDynamicStates = DynamicStates.data();

        VkPipelineViewportStateCreateInfo ViewportState{};
        ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        ViewportState.viewportCount = 0;

        VkPipelineRasterizationStateCreateInfo Rasterizer{};
        Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        Rasterizer.depthClampEnable = VK_FALSE;
        Rasterizer.rasterizerDiscardEnable = VK_FALSE;
        Rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        Rasterizer.lineWidth = 1.f;
        Rasterizer.cullMode = VK_CULL_MODE_NONE;
        Rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        Rasterizer.depthBiasEnable = VK_FALSE;
        Rasterizer.depthBiasConstantFactor = 0.f;
        Rasterizer.depthBiasClamp = 0.f;
        Rasterizer.depthBiasSlopeFactor = 0.f;

        VkPipelineMultisampleStateCreateInfo Multisampling{};
        Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        Multisampling.sampleShadingEnable = VK_FALSE;
        Multisampling.rasterizationSamples = GetVKSampleCount(m_msaaSamples);
        Multisampling.minSampleShading = 1.f;
        Multisampling.pSampleMask = nullptr;
        Multisampling.alphaToCoverageEnable = VK_FALSE;
        Multisampling.alphaToOneEnable = VK_FALSE;
        Multisampling.sampleShadingEnable = VK_TRUE;
        Multisampling.minSampleShading = .2f;

        VkPipelineColorBlendAttachmentState ColorBlendAttachment{};
        ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        ColorBlendAttachment.blendEnable = VK_FALSE;
        ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo ColorBlending{};
        ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        ColorBlending.logicOpEnable = VK_FALSE;
        ColorBlending.logicOp = VK_LOGIC_OP_COPY;
        ColorBlending.attachmentCount = 1;
        ColorBlending.pAttachments = &ColorBlendAttachment;

        VkPipelineLayoutCreateInfo PipelineLayoutInfo{};
        PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        PipelineLayoutInfo.setLayoutCount = 1;
        PipelineLayoutInfo.pSetLayouts = &m_graphicsDescriptorSetLayout;
        PipelineLayoutInfo.pushConstantRangeCount = 0;
        PipelineLayoutInfo.pPushConstantRanges = nullptr;

        VK_CHECK(vkCreatePipelineLayout(m_device, &PipelineLayoutInfo, nullptr,
                                        &m_graphicsPipelineLayout));

        // Depth-Stencil Test
        VkPipelineDepthStencilStateCreateInfo DepthStencil{};
        DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        DepthStencil.depthTestEnable = VK_TRUE;
        DepthStencil.depthWriteEnable = VK_TRUE;
        DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        DepthStencil.depthBoundsTestEnable = VK_FALSE;
        //        DepthStencil.minDepthBounds = 0.f;
        //        DepthStencil.maxDepthBounds = 1.f;
        DepthStencil.stencilTestEnable = VK_FALSE;
        DepthStencil.front = {};
        DepthStencil.back = {};

        VkGraphicsPipelineCreateInfo PipelineInfo{};
        PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        PipelineInfo.stageCount = ShaderStages.size();
        PipelineInfo.pStages = ShaderStages.data();
        PipelineInfo.pVertexInputState = &VertexInputInfo;
        PipelineInfo.pInputAssemblyState = &InputAssemble;
        PipelineInfo.pViewportState = &ViewportState;
        PipelineInfo.pRasterizationState = &Rasterizer;
        PipelineInfo.pMultisampleState = &Multisampling;
        PipelineInfo.pDepthStencilState = &DepthStencil;
        PipelineInfo.pColorBlendState = &ColorBlending;
        PipelineInfo.pDynamicState = &DynamicState;
        PipelineInfo.layout = m_graphicsPipelineLayout;
        PipelineInfo.renderPass = m_renderPass;
        PipelineInfo.subpass = 0;
        PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        PipelineInfo.basePipelineIndex = -1;

        VK_CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr,
                                           &m_graphicsPipeline));
    }

    void VulkanBackendApp::CreateParticleGraphicsPipeline() {
        ShaderBase VertexShader(ShaderType::Vertex, "../../shader/HLSL/ParticleVert.spv", "VSMain");
        ShaderBase FragmentShader(ShaderType::Fragment, "../../shader/HLSL/ParticleFrag.spv",
                                  "PSMain");

        VkPipelineShaderStageCreateInfo VertShaderStageInfo{};
        VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        VertShaderStageInfo.module = VertexShader.GetHandle();
        VertShaderStageInfo.pName = VertexShader.GetEntryName();
        VkPipelineShaderStageCreateInfo FragShaderStageInfo{};
        FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        FragShaderStageInfo.module = FragmentShader.GetHandle();
        FragShaderStageInfo.pName = FragmentShader.GetEntryName();

        std::array<VkPipelineShaderStageCreateInfo, 2> ShaderStages = {
                VertShaderStageInfo, FragShaderStageInfo
        };

        // TODO: Use Particle Vertex Layout and Set Layout in VertexBuffer(Create VertexBufferLayout Class)
        VkPipelineVertexInputStateCreateInfo VertexInputInfo{};
        VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        m_particleVertexBufferLayout = std::make_shared<VertexBufferLayout>(
                std::initializer_list<VertexAttribute>(
                        {
                                VertexAttribute(VertexAttributeDataType::Float3, "Pos"),
                                VertexAttribute(VertexAttributeDataType::Float3, "PlaceHolder"),
                                VertexAttribute(VertexAttributeDataType::Float3, "Color")
                        }));
        auto bindingDescription = m_particleVertexBufferLayout->GetBindingDescription();
        auto attributeDescription = m_particleVertexBufferLayout->GetAttributeDescriptions();
        VertexInputInfo.vertexBindingDescriptionCount = 1;
        VertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        VertexInputInfo.vertexAttributeDescriptionCount = attributeDescription.size();
        VertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

        VkPipelineInputAssemblyStateCreateInfo InputAssemble{};
        InputAssemble.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        InputAssemble.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        InputAssemble.primitiveRestartEnable = VK_FALSE;

        std::vector<VkDynamicState> DynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,
                VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT
        };
        VkPipelineDynamicStateCreateInfo DynamicState{};
        DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        DynamicState.dynamicStateCount = DynamicStates.size();
        DynamicState.pDynamicStates = DynamicStates.data();

        VkPipelineViewportStateCreateInfo ViewportState{};
        ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        ViewportState.viewportCount = 0;

        VkPipelineRasterizationStateCreateInfo Rasterizer{};
        Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        Rasterizer.depthClampEnable = VK_FALSE;
        Rasterizer.rasterizerDiscardEnable = VK_FALSE;
        Rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        Rasterizer.lineWidth = 1.f;
        Rasterizer.cullMode = VK_CULL_MODE_NONE;
        Rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        Rasterizer.depthBiasEnable = VK_FALSE;
        Rasterizer.depthBiasConstantFactor = 0.f;
        Rasterizer.depthBiasClamp = 0.f;
        Rasterizer.depthBiasSlopeFactor = 0.f;

        VkPipelineMultisampleStateCreateInfo Multisampling{};
        Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        Multisampling.sampleShadingEnable = VK_FALSE;
        Multisampling.rasterizationSamples = GetVKSampleCount(m_msaaSamples);
        Multisampling.minSampleShading = 1.f;
        Multisampling.pSampleMask = nullptr;
        Multisampling.alphaToCoverageEnable = VK_FALSE;
        Multisampling.alphaToOneEnable = VK_FALSE;
        Multisampling.sampleShadingEnable = VK_TRUE;
        Multisampling.minSampleShading = .2f;

        VkPipelineColorBlendAttachmentState ColorBlendAttachment{};
        ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        ColorBlendAttachment.blendEnable = VK_FALSE;
        ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo ColorBlending{};
        ColorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        ColorBlending.logicOpEnable = VK_FALSE;
        ColorBlending.logicOp = VK_LOGIC_OP_COPY;
        ColorBlending.attachmentCount = 1;
        ColorBlending.pAttachments = &ColorBlendAttachment;

        // Depth-Stencil Test
        VkPipelineDepthStencilStateCreateInfo DepthStencil{};
        DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        DepthStencil.depthTestEnable = VK_TRUE;
        DepthStencil.depthWriteEnable = VK_TRUE;
        DepthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        DepthStencil.depthBoundsTestEnable = VK_FALSE;
        //        DepthStencil.minDepthBounds = 0.f;
        //        DepthStencil.maxDepthBounds = 1.f;
        DepthStencil.stencilTestEnable = VK_FALSE;
        DepthStencil.front = {};
        DepthStencil.back = {};

        VkGraphicsPipelineCreateInfo PipelineInfo{};
        PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        PipelineInfo.stageCount = ShaderStages.size();
        PipelineInfo.pStages = ShaderStages.data();
        PipelineInfo.pVertexInputState = &VertexInputInfo;
        PipelineInfo.pInputAssemblyState = &InputAssemble;
        PipelineInfo.pViewportState = &ViewportState;
        PipelineInfo.pRasterizationState = &Rasterizer;
        PipelineInfo.pMultisampleState = &Multisampling;
        PipelineInfo.pDepthStencilState = &DepthStencil;
        PipelineInfo.pColorBlendState = &ColorBlending;
        PipelineInfo.pDynamicState = &DynamicState;
        PipelineInfo.layout = m_graphicsPipelineLayout;
        PipelineInfo.renderPass = m_renderPass;
        PipelineInfo.subpass = 0;
        PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        PipelineInfo.basePipelineIndex = -1;

        VK_CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr,
                                           &m_particleGraphicsPipeline));
    }

    void VulkanBackendApp::CreateComputeDescriptorSetLayout() {
        std::array<VkDescriptorSetLayoutBinding, 3> LayoutBindings{};
        LayoutBindings[0].binding = 0;
        LayoutBindings[0].descriptorCount = 1;
        LayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        LayoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        LayoutBindings[1].binding = 1;
        LayoutBindings[1].descriptorCount = 1;
        LayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        LayoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        LayoutBindings[2].binding = 2;
        LayoutBindings[2].descriptorCount = 1;
        LayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        LayoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo LayoutInfo{};
        LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        LayoutInfo.bindingCount = LayoutBindings.size();
        LayoutInfo.pBindings = LayoutBindings.data();

        VK_CHECK(vkCreateDescriptorSetLayout(m_device, &LayoutInfo, nullptr,
                                             &m_computeDescriptorSetLayout));
    }

    void VulkanBackendApp::CreateComputePipeline() {
        VkPipelineLayoutCreateInfo PipelineLayoutInfo{};
        PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        PipelineLayoutInfo.setLayoutCount = 1;
        PipelineLayoutInfo.pSetLayouts = &m_computeDescriptorSetLayout;
        VK_CHECK(vkCreatePipelineLayout(m_device, &PipelineLayoutInfo, nullptr,
                                        &m_computePipelineLayout));

        ShaderBase ComputeShader(ShaderType::Compute, "../../shader/HLSL/UpdateParticle.spv",
                                 "UpdateParticles");

        VkPipelineShaderStageCreateInfo ComputeShaderStageInfo{};
        ComputeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ComputeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        ComputeShaderStageInfo.module = ComputeShader.GetHandle();
        ComputeShaderStageInfo.pName = ComputeShader.GetEntryName();

        VkComputePipelineCreateInfo PipelineInfo{};
        PipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        PipelineInfo.stage = ComputeShaderStageInfo;
        PipelineInfo.layout = m_computePipelineLayout;

        VK_CHECK(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr,
                                          &m_computePipeline));
    }

    void VulkanBackendApp::CreateViewUniformBuffers() {
        glm::vec3 CameraPos = glm::vec3(0.f, 0.f, 3.f);
        float AspectRatio = static_cast<float>(m_windowWidth) / static_cast<float>(m_windowHeight);
        m_camera = std::make_shared<PerspectiveCamera>(CameraPos, AspectRatio, .2f, 1e3f, 45);
        m_camera->Init();

        ViewUniformBuffer ViewUniformBuffer_{};
        ViewUniformBuffer_.ViewTrans = m_camera->GetViewMatrix();
        ViewUniformBuffer_.ProjTrans = m_camera->GetProjMatrix();
        ViewUniformBuffer_.CameraPos = m_camera->GetCameraPos();
        ViewUniformBuffer_.DeltaTime = m_fpsCalculator
                                       ? static_cast<float>(m_fpsCalculator->GetDeltaTime())
                                       : 0.f;
        ViewUniformBuffer_.AccumulatedFrameNum = m_accumulatedFrameNum;
        ViewUniformBuffer_.FrameNum = m_frameNum;
        ViewUniformBuffer_.InvView = glm::transpose(m_camera->GetViewMatrix());
        ViewUniformBuffer_.InvProj = glm::inverse(m_camera->GetProjMatrix());

        m_ViewUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_ViewUniformBuffers[i] = new UniformBuffer(sizeof(ViewUniformBuffer),
                                                        &ViewUniformBuffer_);
        }
    }

    void VulkanBackendApp::CreateDescriptorPool() {
        // TODO: PoolSize
        std::array<VkDescriptorPoolSize, 6> PoolSizes{};
        PoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        PoolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT * 30;
        PoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        PoolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT * 30;
        PoolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        PoolSizes[2].descriptorCount = MAX_FRAMES_IN_FLIGHT * 30;
        PoolSizes[3].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        PoolSizes[3].descriptorCount = MAX_FRAMES_IN_FLIGHT * 30;
        PoolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        PoolSizes[4].descriptorCount = MAX_FRAMES_IN_FLIGHT * 30;
        PoolSizes[5].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        PoolSizes[5].descriptorCount = MAX_FRAMES_IN_FLIGHT;

        VkDescriptorPoolCreateInfo PoolInfo{};
        PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        PoolInfo.poolSizeCount = PoolSizes.size();
        PoolInfo.pPoolSizes = PoolSizes.data();
        PoolInfo.maxSets = MAX_FRAMES_IN_FLIGHT * 40;
        PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        VK_CHECK(vkCreateDescriptorPool(m_device, &PoolInfo, nullptr, &m_descriptorPool));
    }

    void VulkanBackendApp::CreateGraphicsDescriptorSets() {
        std::vector<VkDescriptorSetLayout> Layouts(MAX_FRAMES_IN_FLIGHT,
                                                   m_graphicsDescriptorSetLayout);
        VkDescriptorSetAllocateInfo AllocateInfo{};
        AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        AllocateInfo.descriptorPool = m_descriptorPool;
        AllocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        AllocateInfo.pSetLayouts = Layouts.data();

        m_graphicsDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        VK_CHECK(
                vkAllocateDescriptorSets(m_device, &AllocateInfo, m_graphicsDescriptorSets.data()));

        std::array<VkWriteDescriptorSet, 2> DescriptorWrites{};
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo BufferInfo{};
            BufferInfo.buffer = m_ViewUniformBuffers[i]->GetHandle();
            BufferInfo.offset = 0;
            BufferInfo.range = VK_WHOLE_SIZE;
            DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[0].dstSet = m_graphicsDescriptorSets[i];
            DescriptorWrites[0].dstBinding = 0;
            DescriptorWrites[0].dstArrayElement = 0;
            DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            DescriptorWrites[0].descriptorCount = 1;
            DescriptorWrites[0].pBufferInfo = &BufferInfo;

            VkDescriptorImageInfo ImageInfo{};
            ImageInfo.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
            // NOTE: Deprecated Graphics Pipeline
//            ImageInfo.imageView = m_vikingRoom->GetTexture()->CreateSRV();
//            ImageInfo.sampler = Sampler::GetDefaultSample()->GetHandle();
            DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[1].dstSet = m_graphicsDescriptorSets[i];
            DescriptorWrites[1].dstBinding = 1;
            DescriptorWrites[1].dstArrayElement = 0;
            DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            DescriptorWrites[1].descriptorCount = 1;
            DescriptorWrites[1].pImageInfo = &ImageInfo;

            vkUpdateDescriptorSets(m_device, DescriptorWrites.size(), DescriptorWrites.data(), 0,
                                   nullptr);
        }
    }

    void VulkanBackendApp::CreateSyncObjects() {
        m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_frameInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        m_computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        m_computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkSemaphoreCreateInfo SemaphoreInfo{};
            SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo FenceInfo{};
            FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            VK_CHECK(vkCreateSemaphore(m_device, &SemaphoreInfo, nullptr,
                                       &m_imageAvailableSemaphores[i]));
            VK_CHECK(vkCreateSemaphore(m_device, &SemaphoreInfo, nullptr,
                                       &m_renderFinishedSemaphores[i]));
            VK_CHECK(vkCreateFence(m_device, &FenceInfo, nullptr, &m_frameInFlightFences[i]));
            VK_CHECK(vkCreateFence(m_device, &FenceInfo, nullptr, &m_computeInFlightFences[i]));
            VK_CHECK(vkCreateSemaphore(m_device, &SemaphoreInfo, nullptr,
                                       &m_computeFinishedSemaphores[i]));
        }
    }

    void VulkanBackendApp::RecordCommandBuffer(VkCommandBuffer CommandBuffer, uint ImageIndex) {
        VkCommandBufferBeginInfo BeginInfo{};
        BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VK_CHECK(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

        VkRenderPassBeginInfo RenderPassInfo{};
        RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        RenderPassInfo.renderPass = m_renderPass;
        RenderPassInfo.framebuffer = m_swapChainFrameBuffers[ImageIndex];
        RenderPassInfo.renderArea.offset = {0, 0};
        RenderPassInfo.renderArea.extent = m_swapChain.Extent;
        std::array<VkClearValue, 2> ClearValues{};
        ClearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        ClearValues[1].depthStencil = {1.0f, 0};
        RenderPassInfo.clearValueCount = ClearValues.size();
        RenderPassInfo.pClearValues = ClearValues.data();

        vkCmdBeginRenderPass(CommandBuffer, &RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

        ViewUniformBuffer ViewUniformBuffer_{};
        ViewUniformBuffer_.ViewTrans = m_camera->GetViewMatrix();
        ViewUniformBuffer_.ProjTrans = m_camera->GetProjMatrix();
        ViewUniformBuffer_.CameraPos = m_camera->GetCameraPos();
        ViewUniformBuffer_.DeltaTime = m_fpsCalculator
                                       ? static_cast<float>(m_fpsCalculator->GetDeltaTime())
                                       : 0.f;
        ViewUniformBuffer_.AccumulatedFrameNum = m_accumulatedFrameNum;
        ViewUniformBuffer_.FrameNum = m_frameNum;
        ViewUniformBuffer_.InvView = glm::transpose(m_camera->GetViewMatrix());
        ViewUniformBuffer_.InvProj = glm::inverse(m_camera->GetProjMatrix());
        m_ViewUniformBuffers[ImageIndex]->Update(&ViewUniformBuffer_);

        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_graphicsPipelineLayout,
                                0, 1, m_graphicsDescriptorSets.data(), 0, nullptr);

        VkViewport Viewport{};
        Viewport.x = 0.f;
        Viewport.y = 0.f;
        Viewport.width = static_cast<float>(m_swapChain.Extent.width);
        Viewport.height = static_cast<float>(m_swapChain.Extent.height);
        Viewport.minDepth = 0.f;
        Viewport.maxDepth = 1.f;
        vkCmdSetViewportWithCount(CommandBuffer, 1, &Viewport);
        VkRect2D Scissor{};
        Scissor.offset = {0, 0};
        Scissor.extent = m_swapChain.Extent;
        vkCmdSetScissorWithCount(CommandBuffer, 1, &Scissor);

        // NOTE: Deprecated Graphics Pipeline
//        m_vikingRoom->DrawIndexed(CommandBuffer);

        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          m_particleGraphicsPipeline);
        VkDeviceSize Offsets = 0;
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1,
                               &(m_particleStorageBuffers[m_currentFrame]->GetHandle()), &Offsets);
        vkCmdDraw(CommandBuffer, s_particleCount, 1, 0, 0);

        vkCmdEndRenderPass(CommandBuffer);
    }

    void VulkanBackendApp::RecreateSwapChain() {
        glfwGetFramebufferSize(m_window, reinterpret_cast<int *>(&m_windowWidth),
                               reinterpret_cast<int *>(&m_windowHeight));
        while (m_windowWidth == 0 || m_windowHeight == 0) {
            glfwGetFramebufferSize(m_window, reinterpret_cast<int *>(&m_windowWidth),
                                   reinterpret_cast<int *>(&m_windowHeight));
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(m_device);

        CleanUpSwapChain();
        CreateSwapChain();
        delete m_msaaBuffers;
        CreateMSAABuffers();
        CreateFrameBuffers();
    }

    void VulkanBackendApp::InitImGui() {
        m_imguiInfrastructure = new ImGuiInfrastructure(MAX_FRAMES_IN_FLIGHT);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGuiIO &IO = ImGui::GetIO();

        auto Consolas = IO.Fonts->AddFontFromFileTTF("../../asset/font/Consolas-Regular.ttf", 22.f);
        IO.Fonts->Build();
        IO.FontDefault = Consolas;

        ImGui_ImplVulkan_InitInfo InitInfo{};
        InitInfo.Instance = m_instance;
        InitInfo.PhysicalDevice = m_physicalDevice;
        InitInfo.Device = m_device;
        InitInfo.QueueFamily = Utils::FindQueueFamilies().GraphicsFamily.value();
        InitInfo.Queue = m_queue.GraphicsQueue;
        InitInfo.DescriptorPool = m_imguiInfrastructure->m_descriptorPool;
        InitInfo.RenderPass = m_imguiInfrastructure->m_renderPass;
        InitInfo.MinImageCount = MAX_FRAMES_IN_FLIGHT;
        InitInfo.ImageCount = MAX_FRAMES_IN_FLIGHT;
        InitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&InitInfo);
        ImGui_ImplGlfw_InitForVulkan(m_window, true);

        IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        IO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
        IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // IF using Docking Branch
        IO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // IF using Multi View

        auto &Colors = ImGui::GetStyle().Colors;
        Colors[ImGuiCol_WindowBg] = ImVec4{0.1f, 0.105f, 0.11f, 1.0f};

        // Headers
        Colors[ImGuiCol_Header] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        Colors[ImGuiCol_HeaderHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
        Colors[ImGuiCol_HeaderActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Buttons
        Colors[ImGuiCol_Button] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        Colors[ImGuiCol_ButtonHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
        Colors[ImGuiCol_ButtonActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Frame BG
        Colors[ImGuiCol_FrameBg] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};
        Colors[ImGuiCol_FrameBgHovered] = ImVec4{0.3f, 0.305f, 0.31f, 1.0f};
        Colors[ImGuiCol_FrameBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};

        // Tabs
        Colors[ImGuiCol_Tab] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        Colors[ImGuiCol_TabHovered] = ImVec4{0.38f, 0.3805f, 0.381f, 1.0f};
        Colors[ImGuiCol_TabActive] = ImVec4{0.28f, 0.2805f, 0.281f, 1.0f};
        Colors[ImGuiCol_TabUnfocused] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        Colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.2f, 0.205f, 0.21f, 1.0f};

        // Title
        Colors[ImGuiCol_TitleBg] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        Colors[ImGuiCol_TitleBgActive] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
        Colors[ImGuiCol_TitleBgCollapsed] = ImVec4{0.15f, 0.1505f, 0.151f, 1.0f};
    }

    void VulkanBackendApp::CleanUpImGui() {
        delete m_imguiInfrastructure;
    }

    void VulkanBackendApp::Present() {
        VkPresentInfoKHR PresentInfo{};
        PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        PresentInfo.waitSemaphoreCount = 1;
        PresentInfo.pWaitSemaphores = &m_renderFinishedSemaphores[m_currentFrame];
        PresentInfo.swapchainCount = 1;
        PresentInfo.pSwapchains = &m_swapChain.SwapChainHandle;
        PresentInfo.pImageIndices = &m_imageIndex;

        VkResult Result = vkQueuePresentKHR(m_queue.PresentQueue, &PresentInfo);
        if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR) {
            OnWindowResize();
        } else if (Result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain images");
        }
    }

    void VulkanBackendApp::OnWindowResize() {
        RecreateSwapChain();
        m_imguiInfrastructure->RecreateFrameBuffer();
    }

    void VulkanBackendApp::CreateMSAABuffers() {
        m_msaaBuffers = new MSAABuffer();

        m_msaaBuffers->MSAAColorBuffer = new Texture2D(m_swapChain.Extent.width,
                                                       m_swapChain.Extent.height,
                                                       TextureFormat::RGBA_UNORM,
                                                       TextureUsage::ColorAttachmentMSAA,
                                                       GetVKSampleCount(m_msaaSamples));
        m_msaaBuffers->MSAADepthBuffer = new Texture2D(m_swapChain.Extent.width,
                                                       m_swapChain.Extent.height,
                                                       TextureFormat::Depth32,
                                                       TextureUsage::DepthStencilAttachmentMSAA,
                                                       GetVKSampleCount(m_msaaSamples));
    }

    void VulkanBackendApp::CreateParticleStorageBuffers() {
        m_particleStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        std::default_random_engine RndEngine(static_cast<unsigned>(time(nullptr))); // NOLINT
        std::uniform_real_distribution<float> RndDist(0.0f, 1.0f);

        // Init Particles
        struct Particle {
            glm::vec3 Pos;
            glm::vec3 Velocity;
            glm::vec3 Color;
        };
        std::vector<Particle> Particles(s_particleCount);
        for (auto &Part: Particles) {
            float Theta = std::acos(1 - 2.f * RndDist(RndEngine));
            float Phi = RndDist(RndEngine) * 2 * 3.14159265358979323846; // NOLINT
            float X = sin(Theta) * cos(Phi);
            float Y = sin(Theta) * sin(Phi);
            float Z = cos(Theta);
            Part.Pos = glm::vec3(0.f, 0.f, 0.f);
            Part.Velocity = glm::vec3(X, Y, Z);
            Part.Color = glm::vec3(RndDist(RndEngine), RndDist(RndEngine), RndDist(RndEngine));
        }

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            auto ParticleBuffer = new StorageBuffer(sizeof(Particle) * s_particleCount,
                                                    Particles.data());
            m_particleStorageBuffers[i] = ParticleBuffer;
        }
    }

    void VulkanBackendApp::CreateComputeDescriptorSets() {
        std::vector<VkDescriptorSetLayout> Layouts(MAX_FRAMES_IN_FLIGHT,
                                                   m_computeDescriptorSetLayout);
        VkDescriptorSetAllocateInfo AllocateInfo{};
        AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        AllocateInfo.descriptorPool = m_descriptorPool;
        AllocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        AllocateInfo.pSetLayouts = Layouts.data();

        m_computeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        VK_CHECK(vkAllocateDescriptorSets(m_device, &AllocateInfo, m_computeDescriptorSets.data()));

        std::array<VkWriteDescriptorSet, 3> DescriptorWrites{};
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo BufferInfo{};
            BufferInfo.buffer = m_ViewUniformBuffers[i]->GetHandle();
            BufferInfo.offset = 0;
            BufferInfo.range = VK_WHOLE_SIZE;
            DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[0].dstSet = m_computeDescriptorSets[i];
            DescriptorWrites[0].dstBinding = 0;
            DescriptorWrites[0].dstArrayElement = 0;
            DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            DescriptorWrites[0].descriptorCount = 1;
            DescriptorWrites[0].pBufferInfo = &BufferInfo;

            VkDescriptorBufferInfo LastFrameStorageBufferInfo{};
            int LastFrameIndex = i == 0 ? MAX_FRAMES_IN_FLIGHT - 1 : i - 1;
            LastFrameStorageBufferInfo.buffer = m_particleStorageBuffers[LastFrameIndex]->
                    GetHandle();
            LastFrameStorageBufferInfo.offset = 0;
            LastFrameStorageBufferInfo.range = VK_WHOLE_SIZE;
            DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[1].dstSet = m_computeDescriptorSets[i];
            DescriptorWrites[1].dstBinding = 1;
            DescriptorWrites[1].dstArrayElement = 0;
            DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            DescriptorWrites[1].descriptorCount = 1;
            DescriptorWrites[1].pBufferInfo = &LastFrameStorageBufferInfo;

            VkDescriptorBufferInfo CurrentFrameStorageBufferInfo{};
            CurrentFrameStorageBufferInfo.buffer = m_particleStorageBuffers[i]->GetHandle();
            CurrentFrameStorageBufferInfo.offset = 0;
            CurrentFrameStorageBufferInfo.range = VK_WHOLE_SIZE;
            DescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[2].dstSet = m_computeDescriptorSets[i];
            DescriptorWrites[2].dstBinding = 2;
            DescriptorWrites[2].dstArrayElement = 0;
            DescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            DescriptorWrites[2].descriptorCount = 1;
            DescriptorWrites[2].pBufferInfo = &CurrentFrameStorageBufferInfo;

            vkUpdateDescriptorSets(m_device, 3, DescriptorWrites.data(), 0, nullptr);
        }
    }

    void SwapChain::GetImages(VkDevice Device) {
        uint ImageCount = 0;
        vkGetSwapchainImagesKHR(Device, SwapChainHandle, &ImageCount, nullptr);
        SwapChainImages.resize(ImageCount);
        vkGetSwapchainImagesKHR(Device, SwapChainHandle, &ImageCount, SwapChainImages.data());

        RHI::TextureTransitionInput SrcInput, DstInput;
        SrcInput.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
        SrcInput.AccessMask = 0;
        SrcInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
        DstInput.Layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        DstInput.AccessMask = 0;
        DstInput.PipelineStage = VK_PIPELINE_STAGE_NONE;

        auto CommandBuffer = RHI::BeginIntermediateCommandBuffer(QueueType::Graphics);
        for (uint ImageIndex = 0; ImageIndex < SwapChainImages.size(); ImageIndex++) {
            RHI::TransitionTextureLayout(CommandBuffer, SwapChainImages[ImageIndex], 1,
                                         SrcInput, DstInput);
        }
        RHI::SubmitIntermediateCommandBuffer(CommandBuffer, QueueType::Graphics);
    }

    void SwapChain::CreateImageViews(VkDevice Device) {
        SwapChainImageViews.resize(SwapChainImages.size());
        for (size_t Index = 0; Index < SwapChainImageViews.size(); Index++) {
            VkImageViewCreateInfo ViewCreateInfo{};
            ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ViewCreateInfo.image = SwapChainImages[Index];
            ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ViewCreateInfo.format = Format;
            ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ViewCreateInfo.subresourceRange.levelCount = 1;
            ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            ViewCreateInfo.subresourceRange.layerCount = 1;
            VK_CHECK(vkCreateImageView(Device, &ViewCreateInfo, nullptr,
                                       &SwapChainImageViews[Index]));
        }
    }
} // namespace Shadowy
