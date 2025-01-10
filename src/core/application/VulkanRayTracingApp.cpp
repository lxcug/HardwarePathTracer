//
// Created by HUSTLX on 2025/1/5.
//

#include "VulkanRayTracingApp.h"
#include "core/RHI.h"
#include "core/shader/ShaderBase.h"
#include "core/shaderCompiler/CompilerHLSL.h"
#include "core/Utils.h"


namespace HWPT {

    VulkanRayTracingApp::VulkanRayTracingApp() {
        DeviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        DeviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
        DeviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        DeviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        DeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
    }

    void VulkanRayTracingApp::InitVulkan() {
        CreateVkInstance();
        CreateSurface();
        SelectPhysicalDevice();
        CreateLogicalDevice();
        CreateCommandPool();
        CreateCommandBuffers();

        CreateSwapChain();

        CreateRenderPass();
        CreateMSAABuffers();
        CreateFrameBuffers();

        CreateDescriptorPool();

        CreateUniformBuffers();
        CreateModelAndSampler();

        CreateSyncObjects();

        InitRayTracing();
    }

    void VulkanRayTracingApp::CleanUp() {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            delete m_MVPUniformBuffers[i];
        }
        delete m_accelBuilder;
        delete m_RTSBTBuffer;
        delete m_msaaBuffers;
        delete m_vikingRoom;
        delete m_sampler;
        CleanUpImGui();
        CleanUpSwapChain();

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(m_device, m_graphicsInFlightFences[i], nullptr);
        }

        vkDestroyDescriptorSetLayout(m_device, m_RTDescriptorSetLayout, nullptr);
        vkDestroyPipelineLayout(m_device, m_RTPipelineLayout, nullptr);
        vkDestroyPipeline(m_device, m_RTPipeline, nullptr);

        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        delete m_commandPool;
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyDevice(m_device, nullptr);
        vkDestroyInstance(m_instance, nullptr);

        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    void VulkanRayTracingApp::DrawFrame() {
        if (m_frameBufferResized) {
            OnWindowResize();
            m_frameBufferResized = false;
        }

        // Wait for last frame render finish
        vkWaitForFences(m_device, 1, &m_graphicsInFlightFences[m_currentFrame], VK_TRUE,
                        UINT64_MAX);
        vkResetFences(m_device, 1, &m_graphicsInFlightFences[m_currentFrame]);

        // Wait for image available
        VkResult Result = vkAcquireNextImageKHR(m_device, m_swapChain.SwapChainHandle, UINT64_MAX,
                                                m_imageAvailableSemaphores[m_currentFrame],
                                                VK_NULL_HANDLE, &m_imageIndex);
        if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR) {
            OnWindowResize();
        } else if (Result != VK_SUCCESS) {
            throw std::runtime_error("Failed to acquire swap chain images");
        }

        auto CommandBuffer = m_commandPool->BeginCommandBuffer(QueueType::Graphics);
        vkResetCommandBuffer(CommandBuffer, 0);

        VkCommandBufferBeginInfo BeginInfo{};
        BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VK_CHECK(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

        VkClearColorValue ClearColor = {{0.f, 0.f, 0.f, 1.0f}};

        VkImageSubresourceRange SubresourceRange{};
        SubresourceRange.baseMipLevel = 0;
        SubresourceRange.levelCount = 1;
        SubresourceRange.baseArrayLayer = 0;
        SubresourceRange.layerCount = 1;
        SubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        RHI::TextureTransitionInput SrcInput, DstInput;
        SrcInput.Layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        SrcInput.AccessMask = 0;
        SrcInput.PipelineStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DstInput.Layout = VK_IMAGE_LAYOUT_GENERAL;
        DstInput.AccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        DstInput.PipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        RHI::TransitionTextureLayout(CommandBuffer,
                                     m_swapChain.SwapChainImages[m_imageIndex], 1,
                                     SrcInput, DstInput);

        vkCmdClearColorImage(CommandBuffer, m_swapChain.SwapChainImages[m_imageIndex],
                             VK_IMAGE_LAYOUT_GENERAL, &ClearColor, 1,
                             &SubresourceRange);

        // Update ViewUniformBuffer
        ViewUniformBuffer ViewUniformBuffer_{};
        ViewUniformBuffer_.ModelTrans = glm::identity<glm::mat4>();
        ViewUniformBuffer_.ModelTrans = glm::identity<glm::mat4>();
        ViewUniformBuffer_.ViewTrans = m_camera->GetViewMatrix();
        ViewUniformBuffer_.ProjTrans = m_camera->GetProjMatrix();
        ViewUniformBuffer_.CameraPos = m_camera->GetCameraPos();
        ViewUniformBuffer_.DebugColor = glm::vec3(.5f, .9f, .6f);
        ViewUniformBuffer_.DeltaTime = m_fpsCalculator ? static_cast<float>(m_fpsCalculator->GetDeltaTime()) : 0.f;
        ViewUniformBuffer_.FrameNum = m_frameNum;
        ViewUniformBuffer_.InvView = glm::transpose(m_camera->GetViewMatrix());
        ViewUniformBuffer_.InvProj = glm::inverse(m_camera->GetProjMatrix());
        m_MVPUniformBuffers[m_imageIndex]->Update(&ViewUniformBuffer_);

        // TODO: Add RayTracing Code Here
        vkCmdBindPipeline(CommandBuffer,
                          VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                          m_RTPipeline);
        vkCmdBindDescriptorSets(CommandBuffer,
                                VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                                m_RTPipelineLayout,
                                0, 1,
                                &m_RTDescriptorSets[m_imageIndex],
                                0, nullptr);
        auto Extent = m_swapChain.Extent;
        auto RayTraceFunc = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(m_device,
                                                                                        "vkCmdTraceRaysKHR"));
        RayTraceFunc(CommandBuffer,
                     &m_rayGenRegion, &m_missRegion, &m_hitRegion, &m_callRegion,
                     Extent.width, Extent.height, 1);

        SrcInput.Layout = VK_IMAGE_LAYOUT_GENERAL;
        SrcInput.AccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        SrcInput.PipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        DstInput.Layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        DstInput.AccessMask = 0;
        DstInput.PipelineStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        RHI::TransitionTextureLayout(CommandBuffer,
                                     m_swapChain.SwapChainImages[m_imageIndex], 1,
                                     SrcInput, DstInput);

        VK_CHECK(vkEndCommandBuffer(CommandBuffer));

        // Submit Commands
        VkSubmitInfo SubmitInfo{};
        SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        std::array<VkSemaphore, 1> WaitSemaphores = {
                m_imageAvailableSemaphores[m_currentFrame]
        };
        std::array<VkPipelineStageFlags, 2> WaitStages = {
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };
        SubmitInfo.pWaitDstStageMask = WaitStages.data();
        SubmitInfo.waitSemaphoreCount = WaitSemaphores.size();
        SubmitInfo.pWaitSemaphores = WaitSemaphores.data();

        SubmitInfo.commandBufferCount = 1;
        SubmitInfo.pCommandBuffers = &CommandBuffer;
        SubmitInfo.signalSemaphoreCount = 1;
        SubmitInfo.pSignalSemaphores = &m_renderFinishedSemaphores[m_currentFrame];
        VK_CHECK(vkQueueSubmit(m_queue.GraphicsQueue, 1, &SubmitInfo,
                               m_graphicsInFlightFences[m_currentFrame]));
    }

    void VulkanRayTracingApp::CreateSyncObjects() {
        m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_graphicsInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkSemaphoreCreateInfo SemaphoreInfo{};
            SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo FenceInfo{};
            FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            VK_CHECK(vkCreateFence(m_device, &FenceInfo, nullptr, &m_graphicsInFlightFences[i]));
            VK_CHECK(vkCreateSemaphore(m_device, &SemaphoreInfo, nullptr,
                                       &m_imageAvailableSemaphores[i]));
            VK_CHECK(vkCreateSemaphore(m_device, &SemaphoreInfo, nullptr,
                                       &m_renderFinishedSemaphores[i]));
        }
    }

    void VulkanRayTracingApp::InitRayTracing() {
        // Requesting ray tracing properties
        VkPhysicalDeviceProperties2 Props2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        Props2.pNext = &m_RTProps;
        vkGetPhysicalDeviceProperties2(m_physicalDevice, &Props2);

        CreateAccelerationStructure();

        CreateRTDescriptorSets();
        BindRTDescriptorSets();
        CreateRTPipelineLayout();
        CreateRTPipeline();
        CreateRTSBT();
    }

    void VulkanRayTracingApp::DrawImGuiFrame() {
        ImGuiInfrastructure::Begin("Settings");
        ImGui::Text("FPS: %d", m_fpsCalculator->GetFPS());
        static bool BorderlessWindow = false;
        if (ImGui::Checkbox("Borderless Window", &BorderlessWindow)) {
            glfwSetWindowAttrib(m_window, GLFW_DECORATED, !BorderlessWindow);
        }
        static bool FullScreen = false;
        static int WindowWidth, WindowHeight, WindowPosX, WindowPosY;
        if (!FullScreen) {
            glfwGetWindowSize(m_window, &WindowWidth, &WindowHeight);
            glfwGetWindowPos(m_window, &WindowPosX, &WindowPosY);
        }
        if (ImGui::Checkbox("Full Screen", &FullScreen)) {
            if (FullScreen) {
                const GLFWvidmode *Mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
                glfwSetWindowPos(m_window, 0, 0);
                glfwSetWindowSize(m_window, Mode->width, Mode->height);
            } else {
                glfwSetWindowPos(m_window, WindowPosX, WindowPosY);
                glfwSetWindowSize(m_window, WindowWidth, WindowHeight);
            }
        }

        ImGuiInfrastructure::End();
    }

    void VulkanRayTracingApp::CreateRTDescriptorSets() {
        VkDescriptorSetLayoutBinding ViewUniformBufferBinding{};
        ViewUniformBufferBinding.binding = 0;
        ViewUniformBufferBinding.descriptorCount = 1;
        ViewUniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ViewUniformBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        VkDescriptorSetLayoutBinding TLASBinding{};
        TLASBinding.binding = 1;
        TLASBinding.descriptorCount = 1;
        TLASBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        TLASBinding.stageFlags =
                VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        VkDescriptorSetLayoutBinding OutImageBinding{};
        OutImageBinding.binding = 2;
        OutImageBinding.descriptorCount = 1;
        OutImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        OutImageBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        std::array<VkDescriptorSetLayoutBinding, 3> Bindings = {
                ViewUniformBufferBinding, TLASBinding, OutImageBinding
        };

        VkDescriptorSetLayoutCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        CreateInfo.bindingCount = Bindings.size();
        CreateInfo.pBindings = Bindings.data();

        VK_CHECK(vkCreateDescriptorSetLayout(m_device, &CreateInfo, nullptr,
                                             &m_RTDescriptorSetLayout));

        std::vector<VkDescriptorSetLayout> Layouts(MAX_FRAMES_IN_FLIGHT,
                                                   m_RTDescriptorSetLayout);
        VkDescriptorSetAllocateInfo AllocateInfo{};
        AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        AllocateInfo.descriptorPool = m_descriptorPool;
        AllocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        AllocateInfo.pSetLayouts = Layouts.data();

        m_RTDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        VK_CHECK(vkAllocateDescriptorSets(m_device, &AllocateInfo, m_RTDescriptorSets.data()));
    }

    void VulkanRayTracingApp::BindRTDescriptorSets() {
        std::array<VkWriteDescriptorSet, 3> DescriptorWrites{};
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo BufferInfo{};
            BufferInfo.buffer = m_MVPUniformBuffers[i]->GetHandle();
            BufferInfo.offset = 0;
            BufferInfo.range = VK_WHOLE_SIZE;
            DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[0].dstSet = m_RTDescriptorSets[i];
            DescriptorWrites[0].dstBinding = 0;
            DescriptorWrites[0].dstArrayElement = 0;
            DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            DescriptorWrites[0].descriptorCount = 1;
            DescriptorWrites[0].pBufferInfo = &BufferInfo;

            VkWriteDescriptorSetAccelerationStructureKHR ASInfo{
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
            ASInfo.accelerationStructureCount = 1;
            auto TLASHandle = m_accelBuilder->GetTLAS();
            ASInfo.pAccelerationStructures = &TLASHandle;
            DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[1].dstSet = m_RTDescriptorSets[i];
            DescriptorWrites[1].dstBinding = 1;
            DescriptorWrites[1].dstArrayElement = 0;
            DescriptorWrites[1].descriptorCount = 1;
            DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            DescriptorWrites[1].pNext = &ASInfo;

            VkDescriptorImageInfo ImageInfo{};
            ImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            ImageInfo.imageView = m_swapChain.SwapChainImageViews[i];
            DescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[2].dstSet = m_RTDescriptorSets[i];
            DescriptorWrites[2].dstBinding = 2;
            DescriptorWrites[2].dstArrayElement = 0;
            DescriptorWrites[2].descriptorCount = 1;
            DescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            DescriptorWrites[2].pImageInfo = &ImageInfo;

            vkUpdateDescriptorSets(m_device, DescriptorWrites.size(), DescriptorWrites.data(), 0, nullptr);
        }
    }

    void VulkanRayTracingApp::CreateRTPipelineLayout() {
        // TODO: Light Info as push constants
//        VkPushConstantRange PushConstant{};

        VkPipelineLayoutCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        CreateInfo.setLayoutCount = 1;
        CreateInfo.pSetLayouts = &m_RTDescriptorSetLayout;

        VK_CHECK(vkCreatePipelineLayout(m_device, &CreateInfo, nullptr, &m_RTPipelineLayout));
    }

    void VulkanRayTracingApp::CreateRTPipeline() {
        HLSLCompiler::CompileShader("RayTracing/RayGen.hlsl", "main", ShaderType::RayGen,
                                    "RayTracing/RayGen");
        HLSLCompiler::CompileShader("RayTracing/Miss.hlsl", "main", ShaderType::Miss,
                                    "RayTracing/Miss");
        HLSLCompiler::CompileShader("RayTracing/ClosestHit.hlsl", "main", ShaderType::ClosestHit,
                                    "RayTracing/ClosestHit");

        // Create Shader Modules
        ShaderBase RayGenShader(ShaderType::RayGen, "../../shader/HLSL/RayTracing/RayGen.spv",
                                "main");
        ShaderBase MissShader(ShaderType::Miss, "../../shader/HLSL/RayTracing/Miss.spv", "main");
        ShaderBase ClosestHitShader(ShaderType::ClosestHit,
                                    "../../shader/HLSL/RayTracing/ClosestHit.spv", "main");

        std::array<VkPipelineShaderStageCreateInfo, 3> ShaderStages{};
        VkPipelineShaderStageCreateInfo ShaderStage{};
        ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        ShaderStage.module = RayGenShader.GetHandle();
        ShaderStage.pName = RayGenShader.GetEntryName();
        ShaderStages[RayGen] = ShaderStage;
        ShaderStage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
        ShaderStage.module = MissShader.GetHandle();
        ShaderStage.pName = MissShader.GetEntryName();
        ShaderStages[Miss] = ShaderStage;
        ShaderStage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        ShaderStage.module = ClosestHitShader.GetHandle();
        ShaderStage.pName = ClosestHitShader.GetEntryName();
        ShaderStages[ClosestHit] = ShaderStage;

        // Create Shader Group
        VkRayTracingShaderGroupCreateInfoKHR ShaderGroupCreateInfo{};
        ShaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        ShaderGroupCreateInfo.generalShader = VK_SHADER_UNUSED_KHR;
        ShaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
        ShaderGroupCreateInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
        ShaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

        ShaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        ShaderGroupCreateInfo.generalShader = RayGen;
        m_RTShaderGroups.push_back(ShaderGroupCreateInfo);

        ShaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        ShaderGroupCreateInfo.generalShader = Miss;
        m_RTShaderGroups.push_back(ShaderGroupCreateInfo);

        ShaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        ShaderGroupCreateInfo.generalShader = ClosestHit;
        m_RTShaderGroups.push_back(ShaderGroupCreateInfo);

        VkRayTracingPipelineCreateInfoKHR CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        CreateInfo.stageCount = ShaderStages.size();
        CreateInfo.pStages = ShaderStages.data();
        CreateInfo.groupCount = m_RTShaderGroups.size();
        CreateInfo.pGroups = m_RTShaderGroups.data();
        CreateInfo.layout = m_RTPipelineLayout;
        CreateInfo.maxPipelineRayRecursionDepth = 10;

        auto CreateRTPipelineFunc = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(
                m_device, "vkCreateRayTracingPipelinesKHR"));
        VK_CHECK(CreateRTPipelineFunc(m_device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1,
                                      &CreateInfo, nullptr, &m_RTPipeline));
    }

    void VulkanRayTracingApp::CreateRTSBT() {
        uint RayGenCount = 1, MissCount = 1, HitCount = 1;
        uint HandleCount = RayGenCount + MissCount + HitCount;

        uint HandleSizeAligned = Utils::Align(m_RTProps.shaderGroupHandleSize,
                                              m_RTProps.shaderGroupBaseAlignment);

        m_rayGenRegion.stride = Utils::Align(HandleSizeAligned,
                                             m_RTProps.shaderGroupBaseAlignment);
        m_rayGenRegion.size = m_rayGenRegion.stride;

        m_missRegion.stride = HandleSizeAligned;
        m_missRegion.size = Utils::Align(MissCount * HandleSizeAligned,
                                         m_RTProps.shaderGroupBaseAlignment);

        m_hitRegion.stride = HandleSizeAligned;
        m_hitRegion.size = Utils::Align(HitCount * HandleSizeAligned,
                                        m_RTProps.shaderGroupBaseAlignment);

        uint DataSize = HandleCount * m_RTProps.shaderGroupHandleSize;
        std::vector<uint8_t> Handles(DataSize);
        auto Func = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(
                m_device, "vkGetRayTracingShaderGroupHandlesKHR"));
        VK_CHECK(Func(m_device, m_RTPipeline, 0, HandleCount, DataSize, Handles.data()));

        VkDeviceSize SBTSize = m_rayGenRegion.size + m_missRegion.size + m_hitRegion.size;
        m_RTSBTBuffer = new ArbitraryBuffer(SBTSize,
                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                            VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // Find the SBT addresses of each group
        VkDeviceAddress SBTAddress = RHI::GetBufferDeviceAddress(m_RTSBTBuffer->GetHandle());
        m_rayGenRegion.deviceAddress = SBTAddress;
        m_missRegion.deviceAddress = SBTAddress + m_rayGenRegion.size;
        m_hitRegion.deviceAddress = SBTAddress + m_rayGenRegion.size + m_missRegion.size;

        uint HandleIndex = 0;
        void *MappedData;
        vkMapMemory(m_device, m_RTSBTBuffer->GetMemoryHandle(), 0, SBTSize, 0, &MappedData);
        auto pData = reinterpret_cast<uint8_t *>(MappedData);
        for (int i = 0; i < RayGenCount; i++) {
            memcpy(reinterpret_cast<void *>(pData),
                   Handles.data() + HandleIndex++ * m_RTProps.shaderGroupHandleSize,
                   m_RTProps.shaderGroupHandleSize);
            pData += m_rayGenRegion.stride;
        }
        for (int i = 0; i < MissCount; i++) {
            memcpy(reinterpret_cast<void *>(pData), Handles.data() + HandleIndex++ * m_RTProps.shaderGroupHandleSize,
                   m_RTProps.shaderGroupHandleSize);
            pData += m_missRegion.stride;
        }
        for (int i = 0; i < HitCount; i++) {
            memcpy(reinterpret_cast<void *>(pData), Handles.data() + HandleIndex++ * m_RTProps.shaderGroupHandleSize,
                   m_RTProps.shaderGroupHandleSize);
            pData += m_hitRegion.stride;
        }
        vkUnmapMemory(m_device, m_RTSBTBuffer->GetMemoryHandle());
    }

    void VulkanRayTracingApp::CreateAccelerationStructure() {
        m_accelBuilder = new ASBuilder();
        auto TLASInput = m_vikingRoom->GetBLASBuildInput();
        std::vector<BLASBuildInput> BLASBuildVector{TLASInput};
        m_accelBuilder->BuildBLAS(BLASBuildVector);

        auto BLASInput = m_vikingRoom->GetTLASBuildInput();
        BLASInput.accelerationStructureReference = m_accelBuilder->GetBLASDeviceAddress(0);
        std::vector<VkAccelerationStructureInstanceKHR> TLASBuildVector{BLASInput};
        m_accelBuilder->BuildTLAS(TLASBuildVector);
    }

    void VulkanRayTracingApp::OnWindowResize() {
        VulkanBackendApp::OnWindowResize();
        BindRTDescriptorSets();
    }


}  // namespace HWPT
