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
//            std::cout << "Viewport Size: " << m_viewportSize.x << " " << m_viewportSize.y << std::endl;
//            auto Texture = ImGui_ImplVulkan_AddTexture(m_sampler->GetHandle(),
//                                                       m_swapChain.SwapChainImageViews[m_imageIndex],
//                                                       VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);
//            ImGui::Image(Texture, ImVec2(m_viewportSize.x, m_viewportSize.y), ImVec2(0, -1), ImVec2(1, 0));
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
        Check(s_application == nullptr);
        s_application = this;
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
        InitVulkanInfrastructure();

        CreateUniformBuffers();
        CreateModelAndSampler();
        CreateParticleStorageBuffers();

        CreateRasterPass();
        CreateFrameBuffers();
        CreateComputePass();
    }

    void VulkanBackendApp::InitVulkanInfrastructure() {
        CreateVkInstance();
        CreateSurface();
        SelectPhysicalDevice();
        CreateLogicalDevice();
        CreateCommandPool();
        CreateCommandBuffers();
        CreateDescriptorPool();
        CreateSwapChain();
        CreateMSAABuffers();
        m_renderGraph = new RenderGraph();
        m_scene = new Scene();
    }

    void VulkanBackendApp::FrameBufferResizeCallback(GLFWwindow *Window, int Width, int Height) {
        auto App =
                reinterpret_cast<VulkanBackendApp *>(glfwGetWindowUserPointer(Window));
        App->m_frameBufferResized = true;
    }

    void VulkanBackendApp::CleanUp() {
        delete m_scene;
        delete m_renderGraph;
        delete m_msaaBuffers;

        delete m_sampler;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            delete m_MVPUniformBuffers[i];
            delete m_particleStorageBuffers[i];
            delete m_basePassFrameBuffers[i];
        }

        CleanUpImGui();
        delete m_swapChain;

        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        vkDestroyCommandPool(m_device, m_commandPool.GraphicsPool, nullptr);
        vkDestroyCommandPool(m_device, m_commandPool.ComputePool, nullptr);
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyDevice(m_device, nullptr);
        vkDestroyInstance(m_instance, nullptr);

        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    void VulkanBackendApp::DrawFrame() {
        VkResult Result = vkAcquireNextImageKHR(m_device, m_swapChain->GetHandle(), UINT64_MAX,
                                                m_renderGraph->GetImageAvailableSemaphore(
                                                        m_currentFrame),
                                                VK_NULL_HANDLE, &m_imageIndex);
        if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR) {
            OnWindowResize();
        } else if (Result != VK_SUCCESS) {
            throw std::runtime_error("Failed to acquire swap chain images");
        }
        if (m_frameBufferResized) {
            OnWindowResize();
            m_frameBufferResized = false;
        }

        UpdateViewUniformBuffer();
        m_renderGraph->OnNewFrame(m_currentFrame);

        // Update Particle
        uint LastFrameIndex =
                m_currentFrame == 0 ? MAX_FRAMES_IN_FLIGHT - 1 : m_currentFrame - 1;
        auto UpdateParticlePass = m_renderGraph->GetPassByName<ComputePass>("UpdateParticlePass");
        auto UpdateParticlePassParameters = m_renderGraph->GetShaderParameterByName(
                "UpdateParticlePass");
        UpdateParticlePassParameters->SetParameter("ViewUniformBuffer", m_MVPUniformBuffers[0]);
        UpdateParticlePassParameters->SetParameter("ParticlesIn",
                                                   m_particleStorageBuffers[LastFrameIndex]);
        UpdateParticlePassParameters->SetParameter("ParticlesOut",
                                                   m_particleStorageBuffers[m_currentFrame]);
        UpdateParticlePass->SetCurrentCommandBuffer(m_computeCommandBuffers[m_currentFrame]);
        m_renderGraph->AddPass(
                "UpdateParticlePass",
                UpdateParticlePass,
                [this, UpdateParticlePass]() {
                    UpdateParticlePass->BindRenderPass(m_computeCommandBuffers[m_currentFrame]);
                    UpdateParticlePass->Execute((s_particleCount + 255) / 256, 1, 1);
                }
        );

        // Draw Mesh
        auto MeshRasterPass = m_renderGraph->GetPassByName<RasterPass>("MeshRaster");
        auto MeshRasterPassParameters = m_renderGraph->GetShaderParameterByName("MeshRaster");

        MeshRasterPassParameters->SetParameter<UniformBuffer *>("ViewUniformBuffer",
                                                                m_MVPUniformBuffers[0]);
        MeshRasterPass->SetCurrentCommandBuffer(m_graphicsCommandBuffers[m_currentFrame]);
        m_renderGraph->AddPass(
                "MeshRasterPass",
                MeshRasterPass,
                [this, MeshRasterPass]() {
                    for (auto It = m_scene->Begin(); It != m_scene->End(); It++) {
                        (*It)->Bind(MeshRasterPass->GetShaderParameters());
                        MeshRasterPass->BindRenderPass(m_graphicsCommandBuffers[m_currentFrame]);
                        MeshRasterPass->Execute(**It);
                    }
                }
        );

        // Draw Particle
        auto ParticleRasterPass = m_renderGraph->GetPassByName<RasterPass>("ParticleRaster");
        auto ParticleRasterPassParameters = m_renderGraph->GetShaderParameterByName(
                "ParticleRaster");
        ParticleRasterPassParameters->SetParameter<UniformBuffer *>("ViewUniformBuffer",
                                                                    m_MVPUniformBuffers[0]);
        ParticleRasterPass->SetCurrentCommandBuffer(m_graphicsCommandBuffers[m_currentFrame]);
        m_renderGraph->AddPass(
                "ParticleRasterPass",
                ParticleRasterPass,
                [this, ParticleRasterPass]() {
                    ParticleRasterPass->BindRenderPass(m_graphicsCommandBuffers[m_currentFrame]);
                    ParticleRasterPass->Execute(*m_particleStorageBuffers[m_currentFrame],
                                                s_particleCount);
                }
        );

        m_renderGraph->Execute();
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
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char *> Extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#if !BUILD_RELEASE && !BUILD_SHIPPING
        Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        std::cout << "Enabled Extensions:\n";
        for (const auto &ExtensionName: Extensions) {
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

    auto VulkanBackendApp::IsSuitableDevice(VkPhysicalDevice PhysicalDevice) -> bool {
        QueueFamilyIndices Indices = SwapChain::FindQueueFamilies(PhysicalDevice, m_surface);
        bool IsExtensionSupport = IsDeviceExtensionSupport(PhysicalDevice);
        auto SwapChainSupport = SwapChain::QuerySwapChainSupport(PhysicalDevice, m_surface);
        bool IsSwapChainSupport =
                !SwapChainSupport.Formats.empty() && !SwapChainSupport.PresentModes.empty();

        return Indices.IsComplete() && IsExtensionSupport && IsSwapChainSupport;
    }

    void VulkanBackendApp::CreateLogicalDevice() {
        QueueFamilyIndices Indices = SwapChain::FindQueueFamilies(m_physicalDevice, m_surface);

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
        m_swapChain = new SwapChain(m_surface, MAX_FRAMES_IN_FLIGHT);
    }

    void VulkanBackendApp::CreateFrameBuffers() {
        m_basePassFrameBuffers.resize(m_swapChain->GetConcurrentFrames());

        for (size_t Index = 0; Index < m_basePassFrameBuffers.size(); Index++) {
            std::initializer_list<VkImageView> Attachments = {
                    m_msaaBuffers->MSAAColorBuffer->CreateSRV(),
                    m_msaaBuffers->MSAADepthBuffer->CreateSRV(),
                    m_swapChain->GetSwapChainImageViews()[Index]
            };

            auto BasePass = m_renderGraph->GetPassByName<RasterPass>("MeshRaster");
            m_basePassFrameBuffers[Index] = new FrameBuffer(m_swapChain->GetExtent().width,
                                                            m_swapChain->GetExtent().height,
                                                            Attachments,
                                                            BasePass);
        }
    }

    void VulkanBackendApp::CreateCommandPool() {
        QueueFamilyIndices Indices = SwapChain::FindQueueFamilies(m_physicalDevice, m_surface);

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
        VK_CHECK(vkAllocateCommandBuffers(m_device, &GraphicsAllocateInfo,
                                          m_graphicsCommandBuffers.data()));

        VkCommandBufferAllocateInfo ComputeAllocateInfo{};
        ComputeAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ComputeAllocateInfo.commandPool = m_commandPool.ComputePool;
        ComputeAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ComputeAllocateInfo.commandBufferCount = m_computeCommandBuffers.size();

        VK_CHECK(vkAllocateCommandBuffers(m_device, &ComputeAllocateInfo,
                                          m_computeCommandBuffers.data()));
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

        VkSubmitInfo SubmitInfo{};
        SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        SubmitInfo.commandBufferCount = 1;
        SubmitInfo.pCommandBuffers = &CommandBuffer;

        vkQueueSubmit(m_queue.GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_queue.GraphicsQueue);

        vkFreeCommandBuffers(m_device, m_commandPool.GraphicsPool, 1, &CommandBuffer);
    }

    void VulkanBackendApp::CreateUniformBuffers() {
        MVPData MVP{};
        MVP.ModelTrans = glm::identity<glm::mat4>();
        glm::vec3 CameraPos = glm::vec3(0.f, 0.f, 2.f);
        MVP.ViewTrans = glm::lookAt(CameraPos, CameraPos + glm::vec3(0.f, 0.f, -1.f),
                                    glm::vec3(0.f, 1.f, 0.f)) * mat4_cast(glm::quat(glm::vec3(
                glm::radians(0.f),
                glm::radians(0.f),
                glm::radians(0.f)
        )));
        MVP.ProjTrans = glm::perspective(glm::radians(60.f),
                                         static_cast<float>(m_windowWidth) /
                                         static_cast<float>(m_windowHeight),
                                         1e-3f, 1000.f);
        MVP.DebugColor = glm::vec3(.5f, .9f, .6f);
        MVP.DeltaTime = m_fpsCalculator ? static_cast<float>(m_fpsCalculator->GetDeltaTime()) : 0.f;

        m_MVPUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_MVPUniformBuffers[i] = new UniformBuffer(sizeof(MVPData), &MVP);
        }
    }

    void VulkanBackendApp::CreateDescriptorPool() {
        std::array<VkDescriptorPoolSize, 4> PoolSizes{};
        PoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        PoolSizes[0].descriptorCount = 10;
        PoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        PoolSizes[1].descriptorCount = 10;
        PoolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        PoolSizes[2].descriptorCount = 10;
        PoolSizes[3].type = VK_DESCRIPTOR_TYPE_SAMPLER;
        PoolSizes[3].descriptorCount = 10;

        VkDescriptorPoolCreateInfo PoolInfo{};
        PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        PoolInfo.poolSizeCount = PoolSizes.size();
        PoolInfo.pPoolSizes = PoolSizes.data();
        PoolInfo.maxSets = 40;
        PoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        VK_CHECK(vkCreateDescriptorPool(m_device, &PoolInfo, nullptr, &m_descriptorPool));
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

        m_swapChain->OnResize(m_windowWidth, m_windowHeight);
        delete m_msaaBuffers;
        CreateMSAABuffers();
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            std::initializer_list<VkImageView> Attachments = {
                    m_msaaBuffers->MSAAColorBuffer->CreateSRV(),
                    m_msaaBuffers->MSAADepthBuffer->CreateSRV(),
                    m_swapChain->GetSwapChainImageViews()[i]
            };
            m_basePassFrameBuffers[i]->OnResize(m_windowWidth, m_windowHeight, Attachments);
        }
    }

    void VulkanBackendApp::CreateModelAndSampler() {
        m_scene->AddPrimitive("../../asset/viking_room/viking_room.obj",
                              "../../asset/viking_room/viking_room.png", true);
        m_sampler = new Sampler();
    }

    void VulkanBackendApp::InitImGui() {
        m_imguiInfrastructure = new ImGuiInfrastructure(MAX_FRAMES_IN_FLIGHT);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGuiIO &IO = ImGui::GetIO();

        auto Consolas = IO.Fonts->AddFontFromFileTTF("../../asset/font/Consolas-Regular.ttf", 16.f);
        IO.Fonts->Build();
        IO.FontDefault = Consolas;

        ImGui_ImplVulkan_InitInfo InitInfo{};
        InitInfo.Instance = m_instance;
        InitInfo.PhysicalDevice = m_physicalDevice;
        InitInfo.Device = m_device;
        InitInfo.QueueFamily = SwapChain::FindQueueFamilies(m_physicalDevice,
                                                            m_surface).GraphicsFamily.value();
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
        RenderPassInfo.renderArea.extent = m_swapChain->GetExtent();
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
        PresentInfo.pWaitSemaphores = &m_renderGraph->GetImageRenderFinishSemaphore(m_currentFrame);
        PresentInfo.swapchainCount = 1;
        PresentInfo.pSwapchains = &m_swapChain->GetHandle();
        PresentInfo.pImageIndices = &m_imageIndex;

        VkResult Result = vkQueuePresentKHR(m_queue.PresentQueue, &PresentInfo);
        if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR) {
            OnWindowResize();
        } else if (Result != VK_SUCCESS) {
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
            ImGuiViewport *viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
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
        ImGuiIO &io = ImGui::GetIO();
        ImGuiStyle &style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 370.0f;

        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
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

    void VulkanBackendApp::CreateMSAABuffers() {
        m_msaaBuffers = new MSAABuffer();

        m_msaaBuffers->MSAAColorBuffer = new Texture2D(m_swapChain->GetExtent().width,
                                                       m_swapChain->GetExtent().height,
                                                       TextureFormat::RGBA,
                                                       TextureUsage::ColorAttachmentMSAA,
                                                       GetVKSampleCount(m_msaaSamples));
        m_msaaBuffers->MSAADepthBuffer = new Texture2D(m_swapChain->GetExtent().width,
                                                       m_swapChain->GetExtent().height,
                                                       TextureFormat::Depth32,
                                                       TextureUsage::DepthStencilAttachmentMSAA,
                                                       GetVKSampleCount(m_msaaSamples));
    }

    void VulkanBackendApp::CreateParticleStorageBuffers() {
        m_particleStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        std::default_random_engine RndEngine(static_cast<unsigned>(time(nullptr)));  // NOLINT
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
            float Phi = RndDist(RndEngine) * 2 * 3.14159265358979323846;  // NOLINT
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

    void VulkanBackendApp::CreateRasterPass() {
        auto MeshPass = m_renderGraph->AllocateRasterPass("MeshRaster",
                                                          "../../shader/HLSL/Vert.spv", "VSMain",
                                                          "../../shader/HLSL/Frag.spv", "PSMain",
                                                          true, PrimitiveType::Triangle);
        m_renderGraph->AllocateParameters(MeshPass,
                                          {
                                                  {"ViewUniformBuffer", ShaderParameterType::UniformBuffer},
                                                  {"Texture",           ShaderParameterType::Texture2D}
                                          });

        auto ParticlePass = m_renderGraph->AllocateRasterPass("ParticleRaster",
                                                              "../../shader/HLSL/ParticleVert.spv",
                                                              "VSMain",
                                                              "../../shader/HLSL/ParticleFrag.spv",
                                                              "PSMain",
                                                              false,
                                                              PrimitiveType::Point);
        ParticlePass->SetVertexBufferLayout({
                                                    {VertexAttributeDataType::Float3, "Pos"},
                                                    {VertexAttributeDataType::Float3, "PlaceHolder"},
                                                    {VertexAttributeDataType::Float3, "Color"}
                                            });
        m_renderGraph->AllocateParameters(ParticlePass,
                                          {
                                                  {"ViewUniformBuffer",
                                                   ShaderParameterType::UniformBuffer},
                                          });
    }


    void VulkanBackendApp::CreateComputePass() {
        auto UpdateParticlePass = m_renderGraph->AllocateComputePass("UpdateParticlePass",
                                                                     "../../shader/HLSL/UpdateParticle.spv",
                                                                     "UpdateParticles");
        m_renderGraph->AllocateParameters(UpdateParticlePass,
                                          {
                                                  {"ViewUniformBuffer", ShaderParameterType::UniformBuffer},
                                                  {"ParticlesIn",       ShaderParameterType::StorageBuffer},
                                                  {"ParticlesOut",      ShaderParameterType::RWStorageBuffer}
                                          });
    }

    void VulkanBackendApp::UpdateViewUniformBuffer() {
        MVPData MVP{};
        MVP.ModelTrans = glm::identity<glm::mat4>();
        glm::vec3 CameraPos = glm::vec3(0.f, 0.f, 2.f);
        MVP.ViewTrans = glm::lookAt(CameraPos, CameraPos + glm::vec3(0.f, 0.f, -1.f),
                                    glm::vec3(0.f, 1.f, 0.f)) * mat4_cast(glm::quat(glm::vec3(
                glm::radians(0.f),
                glm::radians(0.f),
                glm::radians(0.f)
        )));
        MVP.ProjTrans = glm::perspective(glm::radians(60.f),
                                         static_cast<float>(m_windowWidth) /
                                         static_cast<float>(m_windowHeight),
                                         1e-3f, 1000.f);
        MVP.DebugColor = glm::vec3(.5f, .9f, .6f);
        MVP.DeltaTime = m_fpsCalculator ? static_cast<float>(m_fpsCalculator->GetDeltaTime()) : 0.f;
        m_MVPUniformBuffers[m_imageIndex]->Update(&MVP);
    }
}  // namespace HWPT
