//
// Created by HUSTLX on 2024/10/7.
//

#include "VulkanBackendApp.h"
#include <unordered_set>
#include <algorithm>
#include <core/shader/ShaderBase.h>
#include <core/buffer/VertexBuffer.h>


namespace HWPT {

    void VulkanBackendApp::Run() {
        Check(m_contextInited);

        while (!glfwWindowShouldClose(m_window)) {
            glfwPollEvents();
            DrawFrame();
        }
        vkDeviceWaitIdle(m_device);

        CleanUp();
    }

    void VulkanBackendApp::Init() {
        Check(g_application == nullptr);
        g_application = this;
        InitWindow();
        InitVulkan();
        m_contextInited = true;
    }

    void VulkanBackendApp::DrawFrame() {
        VkCommandBufferBeginInfo BeginInfo{};
        BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VK_CHECK(vkBeginCommandBuffer(m_computeCommandBuffers[m_currentFrame], &BeginInfo));

        auto ComputeCommandBuffer = m_computeCommandBuffers[m_currentFrame];
        vkCmdBindPipeline(ComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);
//        vkCmdBindDescriptorSets(ComputeCommandBuffer,
//                                VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout,
//                                0, 1, m_computeDescriptorSetLayout, 0, 0);
        vkCmdDispatch(ComputeCommandBuffer, m_swapChain.Extent.width / 8, m_swapChain.Extent.height / 8, 1);

        VK_CHECK(vkEndCommandBuffer(ComputeCommandBuffer));
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

    void VulkanBackendApp::FrameBufferResizeCallback(GLFWwindow *Window, int Width, int Height) {
        auto App =
                reinterpret_cast<VulkanBackendApp*>(glfwGetWindowUserPointer(Window));
        App->m_frameBufferResized = true;
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
    }

    void VulkanBackendApp::CleanUp() {
        CleanUpSwapChain();
        vkDestroyDescriptorSetLayout(m_device, m_graphicsDescriptorSetLayout, nullptr);
        vkDestroyPipelineLayout(m_device, m_graphicsPipelineLayout, nullptr);
        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_computeDescriptorSetLayout, nullptr);
        vkDestroyPipelineLayout(m_device, m_computePipelineLayout, nullptr);
        vkDestroyPipeline(m_device, m_computePipeline, nullptr);
        vkDestroyCommandPool(m_device, m_commandPool.GraphicsPool, nullptr);
        vkDestroyCommandPool(m_device, m_commandPool.ComputePool, nullptr);
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
        vkDestroyDevice(m_device, nullptr);
//        vkDestroyInstance(m_instance, nullptr);

        glfwDestroyWindow(m_window);
        glfwTerminate();
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

    std::vector<const char *> VulkanBackendApp::GetRequiredExtensions() {
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
        glfwCreateWindowSurface(
                m_instance, m_window, nullptr, &m_surface
        );
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

    QueueFamilyIndices VulkanBackendApp::FindQueueFamilies(VkPhysicalDevice PhysicalDevice) {
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

    bool VulkanBackendApp::IsDeviceExtensionSupport(VkPhysicalDevice PhysicalDevice) {
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

    SwapChainSupportDetails
    VulkanBackendApp::QuerySwapChainSupport(VkPhysicalDevice PhysicalDevice) {
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

    bool VulkanBackendApp::IsSuitableDevice(VkPhysicalDevice PhysicalDevice) {
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
            Extent = {(uint)Width, (uint)Height};
            VkExtent2D& MinExtent = SwapChainSupport.Capabilities.minImageExtent;
            VkExtent2D& MaxExtent = SwapChainSupport.Capabilities.maxImageExtent;
            Extent.width = std::clamp(Extent.width, MinExtent.width, MaxExtent.width);
            Extent.height = std::clamp(Extent.height, MinExtent.height, MaxExtent.height);
        }

        uint ImageCount = SwapChainSupport.Capabilities.minImageCount;
        ImageCount = std::clamp(SwapChainSupport.Capabilities.minImageCount,
                                SwapChainSupport.Capabilities.minImageCount,
                                SwapChainSupport.Capabilities.maxImageCount);

        VkSwapchainCreateInfoKHR CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        CreateInfo.surface = m_surface;
        CreateInfo.minImageCount = ImageCount;
        CreateInfo.imageFormat = SurfaceFormat.format;
        CreateInfo.imageColorSpace = SurfaceFormat.colorSpace;
        CreateInfo.presentMode = PresentMode;
        CreateInfo.imageExtent = Extent;
        CreateInfo.imageArrayLayers = 1;
        CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

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
    }

    void VulkanBackendApp::CleanUpSwapChain() {
        for (auto FrameBuffer : SwapChainFrameBuffers) {
            vkDestroyFramebuffer(m_device, FrameBuffer, nullptr);
        }
        for (auto ImageView : m_swapChain.SwapChainImageViews) {
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
        ColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference ColorAttachmentRef{};
        ColorAttachmentRef.attachment = 0;
        ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription SubPass{};
        SubPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        SubPass.colorAttachmentCount = 1;
        SubPass.pColorAttachments = &ColorAttachmentRef;

        VkSubpassDependency Dependency{};
        Dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        Dependency.dstSubpass = 0;
        Dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        Dependency.srcAccessMask = 0;
        Dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        Dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        CreateInfo.attachmentCount = 1;
        CreateInfo.pAttachments = &ColorAttachment;
        CreateInfo.subpassCount = 1;
        CreateInfo.pSubpasses = &SubPass;
        CreateInfo.dependencyCount = 1;
        CreateInfo.pDependencies = &Dependency;

        VK_CHECK(vkCreateRenderPass(m_device, &CreateInfo, nullptr, &m_renderPass));
    }

    void VulkanBackendApp::CreateFrameBuffers() {
        SwapChainFrameBuffers.resize(m_swapChain.SwapChainImages.size());

        for (size_t Index = 0; Index < SwapChainFrameBuffers.size(); Index++) {
            VkImageView Attachments[] = {
                    m_swapChain.SwapChainImageViews[Index]
            };

            VkFramebufferCreateInfo CreateInfo{};
            CreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            CreateInfo.renderPass = m_renderPass;
            CreateInfo.width = m_swapChain.Extent.width;
            CreateInfo.height = m_swapChain.Extent.height;
            CreateInfo.layers = 1;
            CreateInfo.attachmentCount = 1;
            CreateInfo.pAttachments = Attachments;

            VK_CHECK(vkCreateFramebuffer(m_device, &CreateInfo, nullptr, &SwapChainFrameBuffers[Index]));
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

    VkCommandBuffer VulkanBackendApp::BeginIntermediateCommand() {
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

        VkDescriptorSetLayoutCreateInfo LayoutInfo{};
        LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        LayoutInfo.bindingCount = 1;
        LayoutInfo.pBindings = &UBOLayoutBinding;

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

        VkPipelineShaderStageCreateInfo ShaderStages[] = {VertShaderStageInfo, FragShaderStageInfo};

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

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = ShaderStages;
        pipelineInfo.pVertexInputState = &VertexInputInfo;
        pipelineInfo.pInputAssemblyState = &InputAssemble;
        pipelineInfo.pViewportState = &ViewportState;
        pipelineInfo.pRasterizationState = &Rasterizer;
        pipelineInfo.pMultisampleState = &Multisampling;
        pipelineInfo.pDepthStencilState = nullptr;
        pipelineInfo.pColorBlendState = &ColorBlending;
        pipelineInfo.pDynamicState = &DynamicState;
        pipelineInfo.layout = m_graphicsPipelineLayout;
        pipelineInfo.renderPass = m_renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        VK_CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline));
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

}
