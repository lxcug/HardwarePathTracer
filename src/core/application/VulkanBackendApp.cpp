//
// Created by HUSTLX on 2024/10/7.
//

#include "VulkanBackendApp.h"
#include <unordered_set>
#include <algorithm>
#include <core/shader/ShaderBase.h>
#include <core/buffer/VertexBuffer.h>
#include "core/RHI.h"


namespace HWPT {

    auto GetVKDevice() -> VkDevice {
        return VulkanBackendApp::GetApplication()->GetVkDevice();
    }

    auto GetVKPhysicalDevice() -> VkPhysicalDevice {
        return VulkanBackendApp::GetApplication()->GetPhysicalDevice();
    }

    void VulkanBackendApp::Run() {
        Check(m_contextInited);

        while (!glfwWindowShouldClose(m_window)) {
            m_fpsCalculator->Tick();

            glfwPollEvents();
            DrawFrame();

            BeginImGui();
            DrawImGuiFrame();
            EndImGui();

            Present();
        }
        vkDeviceWaitIdle(m_device);

        CleanUp();
    }

    void VulkanBackendApp::DrawImGuiFrame() {
        // TODO: Viewport
//        {
//            ImGui::Begin("Viewport");
//            ImVec2 RegionSize = ImGui::GetContentRegionAvail();
//            m_viewportSize = {RegionSize.x, RegionSize.y};
////            std::cout << "Viewport Size: " << m_viewportSize.x << " " << m_viewportSize.y << std::endl;
////            auto Texture = ImGui_ImplVulkan_AddTexture(m_sampler->GetHandle(),
////                                                       m_swapChain.SwapChainImageViews[m_imageIndex],
////                                                       VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);
////            ImGui::Image(Texture, ImVec2(m_viewportSize.x, m_viewportSize.y), ImVec2(0, -1), ImVec2(1, 0));
//            ImGui::End();
//        }
        {
            ImGui::Begin("Settings");

            ImGui::End();
        }
        {
            ImGui::Begin("Statistics");
            ImGui::Text("FPS: %d", m_fpsCalculator->GetFPS());
            ImGui::End();
        }
    }

    void VulkanBackendApp::Init() {
        Check(g_application == nullptr);
        g_application = this;
        InitWindow();
        InitVulkan();
        InitImGui();
        m_contextInited = true;

        m_fpsCalculator = std::make_shared<FPSCalculator>(1.f);
    }

    void VulkanBackendApp::InitWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        m_window = glfwCreateWindow(
                static_cast<int>(m_windowWidth), static_cast<int>(m_windowHeight),
                m_windowTitle.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, FrameBufferResizeCallback);
    }

    void VulkanBackendApp::InitVulkan() {
        CreateVkInstance();
        CreateSurface();
        SelectPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapChain();
        CreateRenderPass();
        CreateFrameBuffers();

        CreateCommandPool();
        CreateCommandBuffers();

        CreateGraphicsDescriptorSetLayout();
        CreateGraphicsPipeline();
        CreateComputeDescriptorSetLayout();
        CreateComputePipeline();

        CreateTextureAndSampler();

        CreateUniformBuffers();
        CreateDescriptorPool();
        CreateDescriptorSets();

        CreateVertexBuffer();
        CreateIndexBuffer();
        CreateSyncObjects();
    }


    void VulkanBackendApp::FrameBufferResizeCallback(GLFWwindow *Window, int Width, int Height) {
        auto App =
                reinterpret_cast<VulkanBackendApp*>(glfwGetWindowUserPointer(Window));
        App->m_frameBufferResized = true;
    }

    void VulkanBackendApp::CleanUp() {
        delete m_vikingRoom;
        delete m_vertexBuffer;
        delete m_indexBuffer;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            delete m_MVPUniformBuffers[i];
        }
        delete m_texture;
        delete m_sampler;
        delete m_depthTexture;

        CleanUpImGui();

        CleanUpSwapChain();
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
        }

        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_graphicsDescriptorSetLayout, nullptr);
        vkDestroyPipelineLayout(m_device, m_graphicsPipelineLayout, nullptr);
        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_computeDescriptorSetLayout, nullptr);
        vkDestroyPipelineLayout(m_device, m_computePipelineLayout, nullptr);
        vkDestroyPipeline(m_device, m_computePipeline, nullptr);
        vkDestroyCommandPool(m_device, m_commandPool.GraphicsPool, nullptr);
        vkDestroyCommandPool(m_device, m_commandPool.ComputePool, nullptr);
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyDevice(m_device, nullptr);
        vkDestroyInstance(m_instance, nullptr);

        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    void VulkanBackendApp::DrawFrame() {
        vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

        if (m_frameBufferResized) {
            OnWindowResize();
            m_frameBufferResized = false;
        }

        VkResult Result = vkAcquireNextImageKHR(m_device, m_swapChain.SwapChainHandle, UINT64_MAX,
                                                m_imageAvailableSemaphores[m_currentFrame],
                                                VK_NULL_HANDLE, &m_imageIndex);
        if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR) {
            OnWindowResize();
        }
        else if (Result != VK_SUCCESS) {
            throw std::runtime_error("Failed to acquire swap chain images");
        }

        vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
        vkResetCommandBuffer(m_graphicsCommandBuffers[m_currentFrame], 0);
        RecordCommandBuffer(m_graphicsCommandBuffers[m_currentFrame], m_imageIndex);

        VkSubmitInfo SubmitInfo{};
        SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        SubmitInfo.waitSemaphoreCount = 1;
        SubmitInfo.pWaitSemaphores = &m_imageAvailableSemaphores[m_currentFrame];
        std::array<VkPipelineStageFlags, 1> WaitStages = {
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };
        SubmitInfo.pWaitDstStageMask = WaitStages.data();
        SubmitInfo.commandBufferCount = 1;
        SubmitInfo.pCommandBuffers = &m_graphicsCommandBuffers[m_currentFrame];
        SubmitInfo.signalSemaphoreCount = 1;
        SubmitInfo.pSignalSemaphores = &m_renderFinishedSemaphores[m_currentFrame];

        VK_CHECK(vkQueueSubmit(m_queue.GraphicsQueue, 1, &SubmitInfo, m_inFlightFences[m_currentFrame]));
    }

    void VulkanBackendApp::CreateVkInstance() {
        VkApplicationInfo AppInfo{};
        AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        AppInfo.pApplicationName = m_windowTitle.c_str();
        AppInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        AppInfo.pEngineName = "No Engine";
        AppInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        AppInfo.apiVersion = VK_API_VERSION_1_0;

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
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> Extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#if !BUILD_RELEASE && !BUILD_SHIPPING
        Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        std::cout << "Enabled Extensions:\n";
        for (const auto& ExtensionName : Extensions) {
            std::cout << "\t" << ExtensionName << std::endl;
        }
        std::cout.flush();

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

        for (auto Device : Devices) {
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

    auto VulkanBackendApp::FindQueueFamilies(VkPhysicalDevice PhysicalDevice) -> QueueFamilyIndices {
        QueueFamilyIndices Indices;

        uint QueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> QueueFamilies(QueueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueFamilyCount, QueueFamilies.data());

        int Index = 0;
        for (const auto& QueueFamily : QueueFamilies) {
            if (QueueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                Indices.GraphicsFamily = Index;
            }
            if (QueueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                Indices.ComputeFamily = Index;
            }

            VkBool32 PresentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(PhysicalDevice, Index, m_surface, &PresentSupport);
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

    auto VulkanBackendApp::IsDeviceExtensionSupport(VkPhysicalDevice PhysicalDevice) -> bool {
        uint ExtensionCount = 0;
        vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &ExtensionCount, nullptr);
        std::vector<VkExtensionProperties> AvailableExtensions(ExtensionCount);
        vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr,
                                             &ExtensionCount, AvailableExtensions.data());

        std::unordered_set<std::string> RequiredExtensionsCopy(DeviceExtensions.begin(), DeviceExtensions.end());
        for (const auto& SupportExtension : AvailableExtensions) {
            RequiredExtensionsCopy.erase(SupportExtension.extensionName);
        }

        return RequiredExtensionsCopy.empty();
    }

    auto
    VulkanBackendApp::QuerySwapChainSupport(VkPhysicalDevice PhysicalDevice) -> SwapChainSupportDetails {
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
        vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, m_surface, &PresentModeCount, nullptr);

        if (PresentModeCount > 0) {
            Details.PresentModes.resize(PresentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, m_surface,
                                                      &PresentModeCount, Details.PresentModes.data());
        }

        return Details;
    }

    auto VulkanBackendApp::IsSuitableDevice(VkPhysicalDevice PhysicalDevice) -> bool {
        QueueFamilyIndices Indices = FindQueueFamilies(PhysicalDevice);
        bool IsExtensionSupport = IsDeviceExtensionSupport(PhysicalDevice);
        auto SwapChainSupport = QuerySwapChainSupport(PhysicalDevice);
        bool IsSwapChainSupport =
                !SwapChainSupport.Formats.empty() && !SwapChainSupport.PresentModes.empty();

        return Indices.IsComplete() && IsExtensionSupport && IsSwapChainSupport;
    }

    void VulkanBackendApp::CreateLogicalDevice() {
        QueueFamilyIndices Indices = FindQueueFamilies(m_physicalDevice);

        std::unordered_set<uint> UniqueQueueFamilies = {
                Indices.GraphicsFamily.value(),
                Indices.ComputeFamily.value(),
                Indices.PresentFamily.value()
        };
        std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
        QueueCreateInfos.reserve(QueueCreateInfos.size());

        float QueuePriority = 1.f;
        for (uint QueueFamily : UniqueQueueFamilies) {
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
        CreateInfo.pEnabledFeatures = &DeviceFeatures;
        CreateInfo.enabledExtensionCount = DeviceExtensions.size();
        CreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

        if (m_enableValidationLayers) {
            CreateInfo.enabledLayerCount = ValidationLayers.size();
            CreateInfo.ppEnabledLayerNames = ValidationLayers.data();
        }

        // Sync2
        VkPhysicalDeviceSynchronization2FeaturesKHR Sync2Feature = {};
        Sync2Feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
        Sync2Feature.synchronization2 = VK_TRUE;
        CreateInfo.pNext = &Sync2Feature;

        VK_CHECK(vkCreateDevice(m_physicalDevice, &CreateInfo, nullptr, &m_device));
        vkGetDeviceQueue(m_device, Indices.GraphicsFamily.value(), 0, &m_queue.GraphicsQueue);
        vkGetDeviceQueue(m_device, Indices.ComputeFamily.value(), 0, &m_queue.ComputeQueue);
        vkGetDeviceQueue(m_device, Indices.PresentFamily.value(), 0, &m_queue.PresentQueue);
    }

    void VulkanBackendApp::CreateSwapChain() {
        SwapChainSupportDetails SwapChainSupport = QuerySwapChainSupport(m_physicalDevice);

        VkSurfaceFormatKHR SurfaceFormat;
        for (auto Format : SwapChainSupport.Formats) {
            if (Format.format == VK_FORMAT_R8G8B8A8_SRGB &&
                Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                SurfaceFormat = Format;
                break;
            }
        }
        VkPresentModeKHR PresentMode;
        for (auto PreMode : SwapChainSupport.PresentModes) {
            if (PreMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                PresentMode = PreMode;
                break;
            }
        }
        VkExtent2D Extent;
        if (SwapChainSupport.Capabilities.currentExtent.width != UINT_MAX) {
            Extent = SwapChainSupport.Capabilities.currentExtent;
        }
        else {
            int Width, Height;
            glfwGetFramebufferSize(m_window, &Width, &Height);
            Extent = {static_cast<uint>(Width), static_cast<uint>(Height)};
            VkExtent2D& MinExtent = SwapChainSupport.Capabilities.minImageExtent;
            VkExtent2D& MaxExtent = SwapChainSupport.Capabilities.maxImageExtent;
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
        CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        QueueFamilyIndices Indices = FindQueueFamilies(m_physicalDevice);
        std::unordered_set<uint> QueueFamilySet = {
                Indices.GraphicsFamily.value(),
                Indices.ComputeFamily.value(),
                Indices.PresentFamily.value()
        };
        if (QueueFamilySet.size() == 1) {
            CreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        else {
            CreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            CreateInfo.queueFamilyIndexCount = QueueFamilySet.size();
            std::vector<uint> QueueIndices(QueueFamilySet.size());
            for (auto Index : QueueFamilySet) {
                QueueIndices.push_back(Index);
            }
            CreateInfo.pQueueFamilyIndices = QueueIndices.data();
        }

        CreateInfo.preTransform = SwapChainSupport.Capabilities.currentTransform;
        CreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        CreateInfo.clipped = VK_TRUE;
        CreateInfo.oldSwapchain = VK_NULL_HANDLE;

        VK_CHECK(vkCreateSwapchainKHR(m_device, &CreateInfo, nullptr, &m_swapChain.SwapChainHandle));
        m_swapChain.Extent = Extent;
        m_swapChain.Format = SurfaceFormat.format;
        m_swapChain.GetImages(m_device);
        m_swapChain.CreateImageViews(m_device);

        delete m_depthTexture;
        m_depthTexture = new Texture2D(Extent.width, Extent.height, TextureFormat::Depth32);
    }

    void VulkanBackendApp::CleanUpSwapChain() {
        for (auto& FrameBuffer : m_swapChainFrameBuffers) {
            vkDestroyFramebuffer(m_device, FrameBuffer, nullptr);
        }
        for (auto& ImageView : m_swapChain.SwapChainImageViews) {
            vkDestroyImageView(m_device, ImageView, nullptr);
        }
        vkDestroySwapchainKHR(m_device, m_swapChain.SwapChainHandle, nullptr);
    }

    void VulkanBackendApp::CreateRenderPass() {
        VkAttachmentDescription ColorAttachment{};
        ColorAttachment.format = m_swapChain.Format;
        ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // TODO ? For ImGui

        VkAttachmentDescription DepthAttachment{};
        DepthAttachment.format = GetVKFormat(TextureFormat::Depth32);
        DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference ColorAttachmentRef{};
        ColorAttachmentRef.attachment = 0;
        ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference DepthAttachmentRef{};
        DepthAttachmentRef.attachment = 1;
        DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription SubPass{};
        SubPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        SubPass.colorAttachmentCount = 1;
        SubPass.pColorAttachments = &ColorAttachmentRef;
        SubPass.pDepthStencilAttachment = &DepthAttachmentRef;

        VkSubpassDependency Dependency{};
        Dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        Dependency.dstSubpass = 0;
        Dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        Dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        Dependency.srcAccessMask = 0;
        Dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> Attachments = {
                ColorAttachment,
                DepthAttachment
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
            std::array<VkImageView, 2> Attachments = {
                    m_swapChain.SwapChainImageViews[Index],
                    m_depthTexture->CreateSRV()
            };

            VkFramebufferCreateInfo CreateInfo{};
            CreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            CreateInfo.renderPass = m_renderPass;
            CreateInfo.width = m_swapChain.Extent.width;
            CreateInfo.height = m_swapChain.Extent.height;
            CreateInfo.layers = 1;
            CreateInfo.attachmentCount = Attachments.size();
            CreateInfo.pAttachments = Attachments.data();

            VK_CHECK(vkCreateFramebuffer(m_device, &CreateInfo, nullptr, &m_swapChainFrameBuffers[Index]));
        }
    }

    void VulkanBackendApp::CreateCommandPool() {
        QueueFamilyIndices Indices = FindQueueFamilies(m_physicalDevice);

        VkCommandPoolCreateInfo GraphicsPoolCreateInfo{};
        GraphicsPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        GraphicsPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        GraphicsPoolCreateInfo.queueFamilyIndex = Indices.GraphicsFamily.value();

        VK_CHECK(vkCreateCommandPool(m_device, &GraphicsPoolCreateInfo,
                                     nullptr, &m_commandPool.GraphicsPool));

        VkCommandPoolCreateInfo ComputePoolCreateInfo{};
        ComputePoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        ComputePoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        ComputePoolCreateInfo.queueFamilyIndex = Indices.ComputeFamily.value();
        VK_CHECK(vkCreateCommandPool(m_device, &ComputePoolCreateInfo,
                                     nullptr, &m_commandPool.ComputePool));
    }

    void VulkanBackendApp::CreateCommandBuffers() {
        m_graphicsCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        m_computeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo GraphicsAllocateInfo{};
        GraphicsAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        GraphicsAllocateInfo.commandPool = m_commandPool.GraphicsPool;
        GraphicsAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        GraphicsAllocateInfo.commandBufferCount = m_graphicsCommandBuffers.size();
        VK_CHECK(vkAllocateCommandBuffers(m_device, &GraphicsAllocateInfo, m_graphicsCommandBuffers.data()));

        VkCommandBufferAllocateInfo ComputeAllocateInfo{};
        ComputeAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ComputeAllocateInfo.commandPool = m_commandPool.ComputePool;
        ComputeAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ComputeAllocateInfo.commandBufferCount = m_computeCommandBuffers.size();

        VK_CHECK(vkAllocateCommandBuffers(m_device, &ComputeAllocateInfo, m_computeCommandBuffers.data()));
    }

    auto VulkanBackendApp::BeginIntermediateCommand() -> VkCommandBuffer {
        VkCommandBufferAllocateInfo AllocateInfo{};
        AllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        AllocateInfo.commandPool = m_commandPool.GraphicsPool;
        AllocateInfo.commandBufferCount = 1;

        VkCommandBuffer CommandBuffer;
        VK_CHECK(vkAllocateCommandBuffers(m_device, &AllocateInfo, &CommandBuffer));

        VkCommandBufferBeginInfo BeginInfo{};
        BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(CommandBuffer, &BeginInfo);

        return CommandBuffer;
    }

    void VulkanBackendApp::EndIntermediateCommand(VkCommandBuffer CommandBuffer) {
        vkEndCommandBuffer(CommandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &CommandBuffer;

        vkQueueSubmit(m_queue.GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_queue.GraphicsQueue);

        vkFreeCommandBuffers(m_device, m_commandPool.GraphicsPool, 1, &CommandBuffer);
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

        VK_CHECK(vkCreateDescriptorSetLayout(m_device, &LayoutInfo, nullptr, &m_graphicsDescriptorSetLayout));
    }

    void VulkanBackendApp::CreateGraphicsPipeline() {
        ShaderBase VertexShader, FragmentShader;
        VertexShader.CreateShaderModule("../../shader/Vert.spv");
        FragmentShader.CreateShaderModule("../../shader/Frag.spv");

        VkPipelineShaderStageCreateInfo VertShaderStageInfo{};
        VertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        VertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        VertShaderStageInfo.module = VertexShader.m_shaderModule;
        VertShaderStageInfo.pName = "main";
        VkPipelineShaderStageCreateInfo FragShaderStageInfo{};
        FragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        FragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        FragShaderStageInfo.module = FragmentShader.m_shaderModule;
        FragShaderStageInfo.pName = "main";

        std::array<VkPipelineShaderStageCreateInfo, 2> ShaderStages = {
                VertShaderStageInfo, FragShaderStageInfo
        };

        VkPipelineVertexInputStateCreateInfo VertexInputInfo{};
        VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        auto bindingDescription = Vertex::GetBindingDescription();
        auto attributeDescription = Vertex::GetAttributeDescriptions();
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
        Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        Multisampling.minSampleShading = 1.f;
        Multisampling.pSampleMask = nullptr;
        Multisampling.alphaToCoverageEnable = VK_FALSE;
        Multisampling.alphaToOneEnable = VK_FALSE;

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

        VK_CHECK(vkCreatePipelineLayout(m_device, &PipelineLayoutInfo, nullptr, &m_graphicsPipelineLayout));

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

        VK_CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &m_graphicsPipeline));
    }

    void VulkanBackendApp::CreateComputeDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding LayoutBinding{};
        LayoutBinding.binding = 0;
        LayoutBinding.descriptorCount = 1;
        LayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        LayoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo LayoutInfo{};
        LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        LayoutInfo.bindingCount = 1;
        LayoutInfo.pBindings = &LayoutBinding;

        VK_CHECK(vkCreateDescriptorSetLayout(m_device, &LayoutInfo, nullptr, &m_computeDescriptorSetLayout));
    }

    void VulkanBackendApp::CreateComputePipeline() {
        VkPipelineLayoutCreateInfo PipelineLayoutInfo{};
        PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        PipelineLayoutInfo.setLayoutCount = 1;
        PipelineLayoutInfo.pSetLayouts = &m_computeDescriptorSetLayout;
        VK_CHECK(vkCreatePipelineLayout(m_device, &PipelineLayoutInfo, nullptr, &m_computePipelineLayout));

        ShaderBase ComputeShader;
        ComputeShader.CreateShaderModule("../../shader/Example.spv");

        VkPipelineShaderStageCreateInfo ComputeShaderStageInfo{};
        ComputeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ComputeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        ComputeShaderStageInfo.module = ComputeShader.m_shaderModule;
        ComputeShaderStageInfo.pName = "main";

        VkComputePipelineCreateInfo PipelineInfo{};
        PipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        PipelineInfo.stage = ComputeShaderStageInfo;
        PipelineInfo.layout = m_computePipelineLayout;

        VK_CHECK(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &m_computePipeline));
    }

    void VulkanBackendApp::CreateVertexBuffer() {
        const std::vector<Vertex> Vertices = {
                {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

                {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
        };
        m_vertexBuffer = new VertexBuffer(sizeof(Vertex) * Vertices.size(), Vertices.data());
    }

    void VulkanBackendApp::CreateIndexBuffer() {
        const std::vector<uint> Indices = {
                0, 1, 2, 2, 3, 0,
                4, 5, 6, 6, 7, 4
        };
        m_indexBuffer = new IndexBuffer(Indices.size(), Indices.data());
    }

    struct MVPData {
        glm::mat4 ModelTrans;
        glm::mat4 ViewTrans;
        glm::mat4 ProjTrans;
        glm::vec3 DebugColor;
    };

    void VulkanBackendApp::CreateUniformBuffers() {
        MVPData MVP{};
        MVP.ModelTrans = glm::identity<glm::mat4>();
        MVP.ViewTrans = glm::lookAt(glm::vec3(1.f, 1.f, 3.f), glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f));
        MVP.ProjTrans = glm::perspective(glm::radians(60.f),
                                         static_cast<float>(m_windowWidth) / static_cast<float>(m_windowHeight),
                                         1e-3f, 1000.f);
        MVP.DebugColor = glm::vec3(.5f, .9f, .6f);

        m_MVPUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_MVPUniformBuffers[i] = new UniformBuffer(sizeof(MVPData), &MVP);
        }
    }

    void VulkanBackendApp::CreateDescriptorPool() {
        std::array<VkDescriptorPoolSize, 2> PoolSizes{};
        PoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        PoolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
        PoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        PoolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;

        VkDescriptorPoolCreateInfo PoolInfo{};
        PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        PoolInfo.poolSizeCount = PoolSizes.size();
        PoolInfo.pPoolSizes = PoolSizes.data();
        PoolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
        VK_CHECK(vkCreateDescriptorPool(m_device, &PoolInfo, nullptr, &m_descriptorPool));
    }

    void VulkanBackendApp::CreateDescriptorSets() {
        std::vector<VkDescriptorSetLayout> Layouts(MAX_FRAMES_IN_FLIGHT, m_graphicsDescriptorSetLayout);
        VkDescriptorSetAllocateInfo AllocateInfo{};
        AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        AllocateInfo.descriptorPool = m_descriptorPool;
        AllocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        AllocateInfo.pSetLayouts = Layouts.data();

        m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        VK_CHECK(vkAllocateDescriptorSets(m_device, &AllocateInfo, m_descriptorSets.data()));

        std::array<VkWriteDescriptorSet, 2> DescriptorWrites{};
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo BufferInfo{};
            BufferInfo.buffer = m_MVPUniformBuffers[i]->GetHandle();
            BufferInfo.offset = 0;
            BufferInfo.range = VK_WHOLE_SIZE;
            DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[0].dstSet = m_descriptorSets[i];
            DescriptorWrites[0].dstBinding = 0;
            DescriptorWrites[0].dstArrayElement = 0;
            DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            DescriptorWrites[0].descriptorCount = 1;
            DescriptorWrites[0].pBufferInfo = &BufferInfo;

            VkDescriptorImageInfo ImageInfo{};
            ImageInfo.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
//            ImageInfo.imageView = m_texture->CreateSRV();
            ImageInfo.imageView = m_vikingRoom->GetTexture()->CreateSRV();
            ImageInfo.sampler = m_sampler->GetHandle();
            DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[1].dstSet = m_descriptorSets[i];
            DescriptorWrites[1].dstBinding = 1;
            DescriptorWrites[1].dstArrayElement = 0;
            DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            DescriptorWrites[1].descriptorCount = 1;
            DescriptorWrites[1].pImageInfo = &ImageInfo;

            vkUpdateDescriptorSets(m_device, 2, DescriptorWrites.data(), 0, nullptr);
        }
    }

    void VulkanBackendApp::CreateSyncObjects() {
        m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkSemaphoreCreateInfo SemaphoreInfo{};
            SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo FenceInfo{};
            FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            VK_CHECK(vkCreateSemaphore(m_device, &SemaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]));
            VK_CHECK(vkCreateSemaphore(m_device, &SemaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]));
            VK_CHECK(vkCreateFence(m_device, &FenceInfo, nullptr, &m_inFlightFences[i]));
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
        m_vikingRoom->Bind(CommandBuffer);

        MVPData MVP{};
        MVP.ModelTrans = glm::identity<glm::mat4>();
        MVP.ViewTrans = glm::lookAt(glm::vec3(1.f, 1.f, 3.f), glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f));
        MVP.ProjTrans = glm::perspective(glm::radians(60.f),
                                         static_cast<float>(m_windowWidth) / static_cast<float>(m_windowHeight),
                                         1e-3f, 1000.f);
        MVP.DebugColor = glm::vec3(.5f, .9f, .6f);
        m_MVPUniformBuffers[ImageIndex]->Update(&MVP);

        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineLayout,
                                0, 1, m_descriptorSets.data(), 0, nullptr);

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
        vkCmdDrawIndexed(CommandBuffer, m_vikingRoom->GetIndexCount(), 1, 0, 0, 0);
        vkCmdEndRenderPass(CommandBuffer);

        VK_CHECK(vkEndCommandBuffer(CommandBuffer));
    }

    void VulkanBackendApp::RecreateSwapChain() {
        glfwGetFramebufferSize(m_window, reinterpret_cast<int*>(&m_windowWidth), reinterpret_cast<int*>(&m_windowHeight));
        while (m_windowWidth == 0 || m_windowHeight == 0) {
            glfwGetFramebufferSize(m_window, reinterpret_cast<int*>(&m_windowWidth), reinterpret_cast<int*>(&m_windowHeight));
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(m_device);

        CleanUpSwapChain();
        CreateSwapChain();
        CreateFrameBuffers();
    }

    void VulkanBackendApp::CreateTextureAndSampler() {
        m_vikingRoom = new Model("../../asset/viking_room/viking_room.obj",
                                 "../../asset/viking_room/viking_room.png");

        m_texture = new Texture2D("../../asset/texture.jpg");
        m_sampler = new Sampler();
        m_sampler->CreateSampler();
    }

    void VulkanBackendApp::InitImGui() {
        m_imguiInfrastructure = new ImGuiInfrastructure(MAX_FRAMES_IN_FLIGHT);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGuiIO& IO = ImGui::GetIO();

        auto Consolas = IO.Fonts->AddFontFromFileTTF("../../asset/font/Consolas-Regular.ttf", 16.f);
        IO.Fonts->Build();
        IO.FontDefault = Consolas;

        ImGui_ImplVulkan_InitInfo InitInfo{};
        InitInfo.Instance = m_instance;
        InitInfo.PhysicalDevice = m_physicalDevice;
        InitInfo.Device = m_device;
        InitInfo.QueueFamily = FindQueueFamilies(m_physicalDevice).GraphicsFamily.value();
        InitInfo.Queue = m_queue.GraphicsQueue;
        InitInfo.DescriptorPool = m_imguiInfrastructure->m_descriptorPool;
        InitInfo.RenderPass = m_imguiInfrastructure->m_renderPass;
        InitInfo.MinImageCount = MAX_FRAMES_IN_FLIGHT;
        InitInfo.ImageCount = MAX_FRAMES_IN_FLIGHT;
        InitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&InitInfo);
        ImGui_ImplGlfw_InitForVulkan(m_window, true);

        IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        IO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch
        IO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // IF using Multi View
    }

    void VulkanBackendApp::BeginImGui() {
        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplVulkan_NewFrame();
        ImGui::NewFrame();
//        EnableWholeScreenDocking();
    }

    void VulkanBackendApp::EndImGui() {
        auto CommandBuffer = BeginIntermediateCommand();

        VkRenderPassBeginInfo RenderPassInfo{};
        RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        RenderPassInfo.renderPass = m_imguiInfrastructure->m_renderPass;
        RenderPassInfo.framebuffer = m_imguiInfrastructure->m_frameBuffers[m_imageIndex];
        RenderPassInfo.renderArea.offset = {0, 0};
        RenderPassInfo.renderArea.extent = m_swapChain.Extent;
        VkClearValue ClearValue = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        RenderPassInfo.clearValueCount = 1;
        RenderPassInfo.pClearValues = &ClearValue;
        vkCmdBeginRenderPass(CommandBuffer, &RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), CommandBuffer);

        vkCmdEndRenderPass(CommandBuffer);
        EndIntermediateCommand(CommandBuffer);

        // For ImGui MultiView
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(m_window);
    }

    void VulkanBackendApp::CleanUpImGui() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
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
        }
        else if (Result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain images");
        }

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void VulkanBackendApp::EnableWholeScreenDocking() {
        static bool dockspaceOpen = true;
        static bool opt_fullscreen_persistant = true;
        bool opt_fullscreen = opt_fullscreen_persistant;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen) {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
        // all active windows docked into it will lose their parent and become undocked.
        // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
        // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // DockSpace
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 370.0f;

        if(io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
        style.WindowMinSize.x = minWinSizeX;
        ImGui::End();
    }

    void VulkanBackendApp::OnWindowResize() {
        RecreateSwapChain();
        m_imguiInfrastructure->RecreateFrameBuffer();
    }

    void SwapChain::GetImages(VkDevice Device) {
        uint ImageCount = 0;
        vkGetSwapchainImagesKHR(Device, SwapChainHandle, &ImageCount, nullptr);
        SwapChainImages.resize(ImageCount);
        vkGetSwapchainImagesKHR(Device, SwapChainHandle, &ImageCount, SwapChainImages.data());
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
            VK_CHECK(vkCreateImageView(Device, &ViewCreateInfo, nullptr, &SwapChainImageViews[Index]));
        }
    }

}  // namespace HWPT
