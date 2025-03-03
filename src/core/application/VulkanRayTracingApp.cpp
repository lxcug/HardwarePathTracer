//
// Created by HUSTLX on 2025/1/5.
//

#include "VulkanRayTracingApp.h"
#include <imgui_internal.h>
#include "core/RHI.h"
#include "core/shader/ShaderBase.h"
#include "core/shader_compiler/HLSLCompiler.h"
#include "core/Utils.h"
#include "ImGuiFileDialog.h"
#include "UILayer.h"
#include "core/Input/Input.h"


namespace Shadowy {
    auto GetTLAS() -> VkAccelerationStructureKHR {
        return dynamic_cast<VulkanRayTracingApp*>(VulkanBackendApp::GetApplication())->GetScene()->GetAccelBuilder()->GetTLAS();
    }

    auto GetScene() -> Scene* {
        return dynamic_cast<VulkanRayTracingApp*>(VulkanBackendApp::GetApplication())->GetScene();
    }

    auto GetViewportImages() -> std::vector<Texture2D*>& {
        return dynamic_cast<VulkanRayTracingApp*>(VulkanBackendApp::GetApplication())->m_globalTexture->GetGlobalTexture(TextureType::SceneColor);
    }

    void CreateGlobalTexture(TextureType Type, uint Width, uint Height, VkFormat Format,
                             VkImageUsageFlags Usage, VkImageLayout InitialLayout) {
        return dynamic_cast<VulkanRayTracingApp*>(VulkanBackendApp::GetApplication())->
                m_globalTexture->CreateGlobalTexture(Type, Width, Height, Format, Usage, InitialLayout);
    }

    auto GetGlobalTexture(TextureType Type, uint ImageIndex) -> Texture2D* {
        return dynamic_cast<VulkanRayTracingApp*>(VulkanBackendApp::GetApplication())->m_globalTexture->GetGlobalTexture(Type, ImageIndex);
    }

    auto GetGlobalTextureHandle() -> GlobalTexture* {
        return dynamic_cast<VulkanRayTracingApp*>(VulkanBackendApp::GetApplication())->m_globalTexture;
    }

    auto GetLastFrameSceneColor() -> Texture2D* {
        return dynamic_cast<VulkanRayTracingApp*>(VulkanBackendApp::GetApplication())->GetLastFrameSceneColor();
    }

    VulkanRayTracingApp::VulkanRayTracingApp(const std::string &Title) : VulkanBackendApp(Title) {
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

        InitImGui();

        CreateRenderPass();
        CreateMSAABuffers();
        CreateFrameBuffers();

        CreateDescriptorPool();

        CreateViewUniformBuffers();

        CreateSyncObjects();

        InitRayTracing();

        // ¸©ÊÓÍ¼Î»ÖÃ
        // m_camera->SetCameraPosition(glm::vec3(-2.29f, 118.09f, 21.67f));
        // m_camera->SetCameraRotation(-1.21311, -0.000476454);
//        m_camera->SetCameraPosition(glm::vec3(-0.66f, 19.46f, 70.f));
//        m_camera->SetCameraPosition(glm::vec3(0, 0, 20));
//        m_camera->SetCameraPosition(glm::vec3(-600, 510, 40));
//        m_camera->SetCameraRotation(-0.13, -1.41);
//        m_camera->SetCameraRotation(-glm::radians(10.f), glm::radians(10.f));
    }

    void VulkanRayTracingApp::Run() {
        Check(m_contextInited);

        while (!glfwWindowShouldClose(m_window)) {
            glfwPollEvents();

            m_fpsCalculator->Tick();

            if (m_frameBufferResized) {
                OnWindowResize();
                m_frameBufferResized = false;
            }

            // NOTE: ResetFrameNum Should Invoke before m_accumulatedFrameNum++
            if (m_camera->IsMoving()) {
                ResetFrameNum();
            }
            if (!m_accumulationPass->GetRenderOptions().EnableAccumulation || m_camera->IsMoving()) {
                ResetFrameNum();
            }
            for (auto &Operation: m_deferredOperations) {
                Operation();
            }
            m_deferredOperations.clear();

            if (m_currentDelayFrames-- > 0) {
                ResetFrameNum();
            }

            UpdateRenderOptions();
            RenderFrame();
            Present();

            if (m_maxRenderTime <= 0.f || m_currentAccumulatedRenderTime < m_maxRenderTime) {
                m_currentAccumulatedRenderTime += m_fpsCalculator->GetDeltaTime();
            }
            if (m_accumulationPass->GetRenderOptions().MaxAccumulatedFrames <= 0 ||
                m_accumulatedFrameNum < m_accumulationPass->GetRenderOptions().MaxAccumulatedFrames) {
                m_accumulatedFrameNum++;
            }
            m_frameNum++;
            m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        }

        vkDeviceWaitIdle(m_device);
        CleanUp();
    }

    void VulkanRayTracingApp::CleanUp() {
        delete m_globalTexture;
        m_gbufferPass->Release();
        delete m_gbufferPass;
        m_accumulationPass->Release();
        delete m_accumulationPass;
        m_bloomPass->Release();
        delete m_bloomPass;

        Sampler::ReleaseSamplers();
        // delete m_restirResource;
        delete m_toneMappingPass;
        delete m_lastFrameViewportImage;
        delete m_RTScene;
        delete m_RTSBTBuffer;
        delete m_msaaBuffers;
        CleanUpImGui();
        CleanUpSwapChain();

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            delete m_ViewUniformBuffers[i];
            vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(m_device, m_frameInFlightFences[i], nullptr);
        }

        vkFreeDescriptorSets(m_device, m_descriptorPool,
                             m_RTDescriptorSets.size(), m_RTDescriptorSets.data());
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

    void VulkanRayTracingApp::RenderFrame() {
        // Wait for frame fence
        vkWaitForFences(m_device, 1, &m_frameInFlightFences[m_currentFrame], VK_TRUE,
                        UINT64_MAX);
        vkResetFences(m_device, 1, &m_frameInFlightFences[m_currentFrame]);

        // Wait for image available
        VkResult Result = vkAcquireNextImageKHR(m_device, m_swapChain.SwapChainHandle, UINT64_MAX,
                                                m_imageAvailableSemaphores[m_currentFrame],
                                                VK_NULL_HANDLE, &m_imageIndex);
        if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR) {
            OnWindowResize();
        } else if (Result != VK_SUCCESS) {
            throw std::runtime_error("Failed to acquire swap chain images");
        }

        auto CommandBuffer = m_computeCommandBuffers[m_currentFrame];
        auto CurrentFrameViewportImage = m_globalTexture->GetGlobalTexture(TextureType::SceneColor, m_currentFrame);

        vkResetCommandBuffer(CommandBuffer, 0);
        VkCommandBufferBeginInfo BeginInfo{};
        BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_CHECK(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

        m_gbufferPass->Dispatch(CommandBuffer, m_imageIndex);

        // Dispatch Ray Tracing Pass
        std::array<VkDescriptorSet, 2> BindingDescriptorSets = {
                m_RTDescriptorSets[m_imageIndex],
                m_RTScene->GetModelDescDescriptorSet(m_imageIndex)
        };
        vkCmdBindPipeline(CommandBuffer,
                          VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                          m_RTPipeline);
        vkCmdBindDescriptorSets(CommandBuffer,
                                VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                                m_RTPipelineLayout,
                                0, BindingDescriptorSets.size(),
                                BindingDescriptorSets.data(),
                                0, nullptr);
        vkCmdPushConstants(CommandBuffer, m_RTPipelineLayout,
                           VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                           VK_SHADER_STAGE_MISS_BIT_KHR,
                           0, sizeof(PathTracingOptions), &m_pathTracingOptions);

        auto RayTraceFunc = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(
                m_device, "vkCmdTraceRaysKHR"));
        RayTraceFunc(CommandBuffer, &m_rayGenRegion, &m_missRegion, &m_hitRegion,
                     &m_callRegion, m_viewportSize.x, m_viewportSize.y, 1);

//        if (m_pathTracingOptions.EnableReSTIRGI) {
//            VkMemoryBarrier ReservoirBufferBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
//            ReservoirBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
//            ReservoirBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//            vkCmdPipelineBarrier(CommandBuffer,
//                                 VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
//                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
//                                 0, 1, &ReservoirBufferBarrier,
//                                 0, nullptr, 0, nullptr);
//
//            m_restirResource->DispatchTemporalReusePass(CommandBuffer, CurrentFrameViewportImage,
//                                                        m_imageIndex,
//                                                        m_viewportSize.x, m_viewportSize.y);
//            m_restirResource->DispatchSpatialReusePass(CommandBuffer, CurrentFrameViewportImage,
//                                                       m_imageIndex,
//                                                       m_viewportSize.x, m_viewportSize.y);
//        }

        RHI::TextureTransitionInput SrcInput{}, DstInput{};
        SrcInput.AccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        SrcInput.PipelineStage = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
        SrcInput.Layout = VK_IMAGE_LAYOUT_GENERAL;
        DstInput.AccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        DstInput.PipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        DstInput.Layout = VK_IMAGE_LAYOUT_GENERAL;
        RHI::TransitionTextureLayout(CommandBuffer,
                                     m_globalTexture->GetGlobalTexture(TextureType::SceneColor, m_imageIndex)->GetHandle(),
                                     1, SrcInput, DstInput);
        m_accumulationPass->Dispatch(CommandBuffer, m_imageIndex);

        if (m_enableBloom)
        {
            SrcInput.AccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            SrcInput.PipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            SrcInput.Layout = VK_IMAGE_LAYOUT_GENERAL;
            DstInput.AccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            DstInput.PipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            DstInput.Layout = VK_IMAGE_LAYOUT_GENERAL;
            RHI::TransitionTextureLayout(CommandBuffer,
                                         m_globalTexture->GetGlobalTexture(TextureType::SceneColor, m_imageIndex)->GetHandle(),
                                         1, SrcInput, DstInput);
            m_bloomPass->Dispatch(CommandBuffer, m_imageIndex);
        }

        RHI::TransitionTextureLayout(CommandBuffer,
                             m_globalTexture->GetGlobalTexture(TextureType::SceneColor, m_imageIndex)->GetHandle(),
                             1, SrcInput, DstInput);
        bool ShouldToneMapping = m_toneMappingPass->GetPassRenderOptions().EnableToneMapping |
                                 m_toneMappingPass->GetPassRenderOptions().EnableGammaCorrection;
        VkImageMemoryBarrier Barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        Barrier.image = CurrentFrameViewportImage->GetHandle();
        Barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        Barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        Barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        Barrier.dstAccessMask = ShouldToneMapping ? VK_ACCESS_SHADER_WRITE_BIT
                                                  : VK_ACCESS_SHADER_READ_BIT;
        Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        Barrier.subresourceRange.baseMipLevel = 0;
        Barrier.subresourceRange.levelCount = 1;
        Barrier.subresourceRange.baseArrayLayer = 0;
        Barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(CommandBuffer,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             ShouldToneMapping ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT :
                             VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr,
                             1, &Barrier);

        if (ShouldToneMapping) {
            m_toneMappingPass->Dispatch(CommandBuffer, m_imageIndex, m_viewportSize.x,
                                        m_viewportSize.y);
        }

        VkMemoryBarrier MemBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        MemBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        MemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(CommandBuffer,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 1, &MemBarrier,
                             0, nullptr, 0, nullptr);

        m_imguiInfrastructure->BeginImGui();
        DrawImGuiFrame();
        m_imguiInfrastructure->EndImGui(CommandBuffer);

        vkEndCommandBuffer(CommandBuffer);

        VkSubmitInfo SubmitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        std::array<VkSemaphore, 1> WaitSemaphores = {
                m_imageAvailableSemaphores[m_currentFrame]
        };
        std::array<VkPipelineStageFlags, 1> WaitStages = {
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };
        SubmitInfo.pWaitDstStageMask = WaitStages.data();
        SubmitInfo.waitSemaphoreCount = WaitSemaphores.size();
        SubmitInfo.pWaitSemaphores = WaitSemaphores.data();
        SubmitInfo.signalSemaphoreCount = 1;
        SubmitInfo.pSignalSemaphores = &m_renderFinishedSemaphores[m_currentFrame];

        SubmitInfo.commandBufferCount = 1;
        SubmitInfo.pCommandBuffers = &CommandBuffer;
        VK_CHECK(vkQueueSubmit(m_commandPool->GetComputeQueue(), 1, &SubmitInfo,
                               m_frameInFlightFences[m_currentFrame]));
    }

    void VulkanRayTracingApp::CreateSyncObjects() {
        m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_frameInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkSemaphoreCreateInfo SemaphoreInfo{};
            SemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo FenceInfo{};
            FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            FenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            VK_CHECK(vkCreateFence(m_device, &FenceInfo, nullptr, &m_frameInFlightFences[i]));

            VK_CHECK(vkCreateSemaphore(m_device, &SemaphoreInfo, nullptr,
                                       &m_imageAvailableSemaphores[i]));
            VK_CHECK(vkCreateSemaphore(m_device, &SemaphoreInfo, nullptr,
                                       &m_renderFinishedSemaphores[i]));
        }
    }

    void VulkanRayTracingApp::InitRayTracing() {
        m_globalTexture = new GlobalTexture();

        // Requesting ray tracing properties
        VkPhysicalDeviceProperties2 Props2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        Props2.pNext = &m_RTProps;
        vkGetPhysicalDeviceProperties2(m_physicalDevice, &Props2);

        CreateViewportImages();

        InitScene();

        // m_restirResource = new ReSTIRResource(m_viewportSize.x, m_viewportSize.y);
        // m_restirResource->CreateTemporalReusePassSets();
        // m_restirResource->CreateTemporalReusePassPipeline();
        // m_restirResource->CreateSpatialReusePassSets();
        // m_restirResource->CreateSpatialReusePassPipeline();

        CreateRTDescriptorSets();
        CreateRTPipelineLayout();
        CreateRTPipeline();
        CreateRTSBT();

        m_gbufferPass = new GBufferPass();
        m_gbufferPass->Init(m_viewportSize.x, m_viewportSize.y);
        m_gbufferPass->CreateSets();
        m_gbufferPass->CreatePipeline();

        m_accumulationPass = new AccumulationPass();
        m_accumulationPass->Init(m_viewportSize.x, m_viewportSize.y);
        m_accumulationPass->CreateSets();
        m_accumulationPass->CreatePipeline();
        m_bloomPass = new BloomPass();
        m_bloomPass->Init(m_viewportSize.x, m_viewportSize.y);
        m_bloomPass->CreateSets();
        m_bloomPass->CreatePipeline();
        m_toneMappingPass = new ToneMappingPass();  // TODO: inherit PassBase

        UpdateRTDescriptorSets();
        m_gbufferPass->UpdateSets();
        m_accumulationPass->UpdateSets();
        m_bloomPass->UpdateSets();
        // m_restirResource->UpdateTemporalReusePassSets();
        // m_restirResource->UpdateSpatialReusePassSets();
        m_toneMappingPass->UpdateDescriptorSets();
    }

    void VulkanRayTracingApp::DrawImGuiFrame() {
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
            ImGuiInfrastructure::Begin("Viewport");
            ImVec2 ViewportSize = ImGui::GetContentRegionAvail();
            ImVec2 ViewportPos = ImGui::GetWindowPos();
            static bool IsWindowHovered = false;
            IsWindowHovered = ImGui::IsWindowHovered();
            static bool IsWindowDocked = false;
            IsWindowDocked = ImGui::IsWindowDocked();
            m_viewportOffset.x = ViewportPos.x;
            m_viewportOffset.y = ViewportPos.y;
            if (ViewportSize.x != m_viewportSize.x || ViewportSize.y != m_viewportSize.y) {
                m_viewportSize.x = ViewportSize.x;
                m_viewportSize.y = ViewportSize.y;
                m_deferredOperations.emplace_back([this]() {
                    ResizeViewportImages();
                    ResetFrameNum();
                });
            }

            if (IsWindowHovered) {
                m_camera->Tick(m_fpsCalculator->GetDeltaTime());
            }

            ImGui::Image(reinterpret_cast<ImTextureID>(m_viewportImageDescriptorSets[m_imageIndex]),
                         {m_viewportSize.x, m_viewportSize.y});

            ImGuiInfrastructure::End();
            ImGui::PopStyleVar();
        }

        static bool ShouldReAccumulate = false;
        ShouldReAccumulate = false;

        {
            ImGuiInfrastructure::Begin("Settings");
            ImGui::Text("FPS: %d", m_fpsCalculator->GetFPS());
            ImGui::Text("Accumulated Frames: %d", m_accumulatedFrameNum);
            ImGui::Text("Render Resolution (%.0f, %.0f)", m_viewportSize.x, m_viewportSize.y);
            static bool FullScreen = false;
            static int WindowWidth, WindowHeight, WindowPosX, WindowPosY;
            if (ImGui::Checkbox("Full Screen", &FullScreen)) {
                glfwSetWindowAttrib(m_window, GLFW_DECORATED, !FullScreen);
                if (FullScreen) {
                    glfwGetWindowSize(m_window, &WindowWidth, &WindowHeight);
                    glfwGetWindowPos(m_window, &WindowPosX, &WindowPosY);
                    const GLFWvidmode *Mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
                    glfwSetWindowPos(m_window, 0, 0);
                    glfwSetWindowSize(m_window, Mode->width, Mode->height);
                } else {
                    glfwSetWindowPos(m_window, WindowPosX, WindowPosY);
                    glfwSetWindowSize(m_window, WindowWidth, WindowHeight);
                }
            }

            if (ImGui::Button("Compile Ray Tracing Shaders") ||
                Input::IsKeyPressed(KeyCode::LeftControl) &&
                Input::IsKeyPressed(KeyCode::LeftAlt) && Input::IsKeyPressed(KeyCode::Period)) {
                m_deferredOperations.emplace_back([this]() {
                    vkDeviceWaitIdle(m_device);
                    ReCompileShaders();
                    ResetFrameNum();
                });
            }

            ImGui::NewLine();
            ImGui::Separator();
            ImGui::NewLine();
            ImGui::Text("Render Options");
            ShouldReAccumulate |= ImGui::Checkbox("Enable AO",
                                                  reinterpret_cast<bool *>(&m_pathTracingOptions.EnableAO));
            ShouldReAccumulate |= ImGui::InputInt("Num AO Rays", &m_pathTracingOptions.NumAORays,
                                                  1.f);
            ShouldReAccumulate |= ImGui::InputFloat("AO Ray Length",
                                                    &m_pathTracingOptions.AORayLength,
                                                    .5f, 1.f, "%.1f");
            ShouldReAccumulate |= ImGui::Checkbox("Enable Gamma Correction",
                                                  reinterpret_cast<bool *>(&m_toneMappingPass->GetPassRenderOptions().EnableGammaCorrection));
            ShouldReAccumulate |= ImGui::Checkbox("Enable ToneMapping",
                                                  reinterpret_cast<bool *>(&m_toneMappingPass->GetPassRenderOptions().EnableToneMapping));
            ShouldReAccumulate |= ImGui::Checkbox("Enable Bloom", &m_enableBloom);
            ShouldReAccumulate |= ImGui::InputFloat("Adapted Luminance",
                                                    &m_toneMappingPass->GetPassRenderOptions().AdaptedLuminance);


            ImGui::NewLine();
            ImGui::Separator();
            ImGui::NewLine();
            ImGui::Text("Path Tracing Options");
            ShouldReAccumulate |= ImGui::Checkbox("Enable ReSTIRGI",
                                                  reinterpret_cast<bool *>(&m_pathTracingOptions.EnableReSTIRGI));
            // ShouldReAccumulate |= ImGui::Checkbox("Enable SpatialReuse",
                                                  // reinterpret_cast<bool *>(&m_restirResource->GetSpatialReuseOptions().EnableSpatialReuse));
            // ShouldReAccumulate |= ImGui::InputInt("Reuse Radius",
                                                  // &m_restirResource->GetSpatialReuseOptions().ReuseRadius,
                                                  // 1);
            ShouldReAccumulate |= ImGui::Checkbox("Show Indirect Only",
                                                  reinterpret_cast<bool *>(&m_pathTracingOptions.ShowIndirectOnly));
            ShouldReAccumulate |= ImGui::Checkbox("Enable Accumulation",
                                                  reinterpret_cast<bool *>(&m_accumulationPass->GetRenderOptions().EnableAccumulation));
            ShouldReAccumulate |= ImGui::InputInt("Max Accumulated Frames",
                                                  &m_accumulationPass->GetRenderOptions().MaxAccumulatedFrames);
            ShouldReAccumulate |= ImGui::InputFloat("Max Render Time In Seconds", &m_maxRenderTime);
            ShouldReAccumulate |= ImGui::Checkbox("Enable Emissive",
                                                  reinterpret_cast<bool *>(&m_pathTracingOptions.EnableEmissive));
            ShouldReAccumulate |= ImGui::Checkbox("Deferred Trace Light",
                                                  reinterpret_cast<bool *>(&m_pathTracingOptions.DeferredTraceMaterial));
            static std::array<const char *, 3> items = {"Sample Light Only",
                                                        "Sample Material Only", "MIS"};
            static int item_current_idx = 2;
            if (ImGui::BeginCombo("Sample Mode", items[item_current_idx])) {
                for (int n = 0; n < items.size(); n++) {
                    const bool is_selected = (item_current_idx == n);
                    if (ImGui::Selectable(items[n], is_selected)) {
                        ShouldReAccumulate = true;
                        item_current_idx = n;
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            m_pathTracingOptions.MISMode = item_current_idx;
            ShouldReAccumulate |= ImGui::InputFloat("Min Ray Bias",
                                                    &m_pathTracingOptions.RayMinBias,
                                                    1e-3f);
            ShouldReAccumulate |= ImGui::InputInt("Bounce", &m_pathTracingOptions.Bounce, 1);
            ShouldReAccumulate |= ImGui::Checkbox("Enable SkyLight",
                                                  reinterpret_cast<bool *>(&m_pathTracingOptions.EnableSkyLight));

            // TODO: Choose Obj
            if (ImGui::Button("Open Scene")) {
                IGFD::FileDialogConfig Config;
                Config.path = "../../asset";
                Config.flags = ImGuiFileDialogFlags_Modal;
                ImGuiFileDialog::Instance()->OpenDialog("ChooseObj", "Choose File",
                                                        ".glb, .gltf, .fbx, .obj", Config);
            }
            if (ImGui::Button("Open Sky Texture")) {
                IGFD::FileDialogConfig Config;
                Config.path = "../../asset/env";
                ImGuiFileDialog::Instance()->OpenDialog("ChooseSkyTexture", "Choose File",
                                                        ".hdr, .jpg, .png", Config);
            }
            if (ImGuiFileDialog::Instance()->Display("ChooseObj")) { // => will show a dialog
                if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
                    std::string FilePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                    std::string FilePath = ImGuiFileDialog::Instance()->GetCurrentPath();
                    m_deferredOperations.emplace_back([this, FilePathName]() {
                        ReloadScene(std::filesystem::path(FilePathName));
                        ResetFrameNum();
                    });
                }
                ImGuiFileDialog::Instance()->Close();
            }
            if (ImGuiFileDialog::Instance()->Display("ChooseSkyTexture")) { // => will show a dialog
                if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
                    std::string FilePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                    std::string FilePath = ImGuiFileDialog::Instance()->GetCurrentPath();
                    vkDeviceWaitIdle(m_device);
                    m_deferredOperations.emplace_back([this, FilePathName]() {
                        ResetFrameNum();
                        m_RTScene->CreateSkyTexture(FilePathName);
                        m_RTScene->UpdateSceneDescDescriptorSets();
                    });
                }
                ImGuiFileDialog::Instance()->Close();
            }
            ImGuiInfrastructure::End();
        }

        {
            ImGuiInfrastructure::Begin("Scene Info");
            ImGui::Text("Camera Info");

            UILayer::DrawFloat3Control("Position", m_camera->GetCameraPos(), 0.f,
                                       &ShouldReAccumulate);

            static bool FOVChanged = false;
            FOVChanged = false;
            auto PerspCamera = std::dynamic_pointer_cast<PerspectiveCamera>(m_camera);
            UILayer::DrawSlideFloatControl("FOV", PerspCamera->GetFOV(), 45.f, &FOVChanged, 150.f,
                                           5.f, 120.f, "%.1f");
            if (FOVChanged) {
                m_deferredOperations.emplace_back([this, PerspCamera]() {
                    ResetFrameNum();
                    PerspCamera->UpdateProjMatrix();
                });
            }

            UILayer::DrawSlideFloatControl("Move Speed", m_camera->GetCameraMoveSpeed(), 1.f,
                                           nullptr,
                                           150.f, 1e-2f, 100.f, "%.2f");
            UILayer::DrawSlideFloatControl("Rotate Speed", m_camera->GetCameraRotateSpeed(), 1.f,
                                           nullptr,
                                           150.f, 1e-2f, 5.f, "%.2f");

            ImGui::NewLine();
            ImGui::Separator();
            ImGui::NewLine();

            ImGui::Text("Light Info");
            bool ShouldUpdateLight = false;
            UILayer::DrawLights(m_RTScene->GetSceneLights(), &ShouldUpdateLight);
            if (ShouldUpdateLight) {
                m_deferredOperations.emplace_back([this]() {
                    ResetFrameNum();
                    m_RTScene->CreateSceneLightsBuffer();
                    m_RTScene->UpdateSceneDescDescriptorSets();
                });
            }

            ImGuiInfrastructure::End();
        }

        if (ShouldReAccumulate) {
            m_deferredOperations.emplace_back([this]() {
                ResetFrameNum();
            });
        }
    }

    void VulkanRayTracingApp::CreateRTDescriptorSets() {
        VkDescriptorSetLayoutBinding ViewUniformBufferBinding{};
        ViewUniformBufferBinding.binding = 0;
        ViewUniformBufferBinding.descriptorCount = 1;
        ViewUniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ViewUniformBufferBinding.stageFlags =
                VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

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

        VkDescriptorSetLayoutBinding InImageBinding{};
        InImageBinding.binding = 3;
        InImageBinding.descriptorCount = 1;
        InImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        InImageBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        VkDescriptorSetLayoutBinding GBufferBinding{};
        GBufferBinding.binding = 4;
        GBufferBinding.descriptorCount = 3;
        GBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        GBufferBinding.stageFlags =
                VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        // VkDescriptorSetLayoutBinding InitialSampleBufferBinding{};
        // InitialSampleBufferBinding.binding = 5;
        // InitialSampleBufferBinding.descriptorCount = 1;
        // InitialSampleBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        // InitialSampleBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        VkDescriptorSetLayoutBinding DiffuseHitDisBinding{};
        DiffuseHitDisBinding.binding = 5;
        DiffuseHitDisBinding.descriptorCount = 1;
        DiffuseHitDisBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        DiffuseHitDisBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        VkDescriptorSetLayoutBinding SpecularHitDisBinding{};
        SpecularHitDisBinding.binding = 6;
        SpecularHitDisBinding.descriptorCount = 1;
        SpecularHitDisBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        SpecularHitDisBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        std::array<VkDescriptorSetLayoutBinding, 7> Bindings = {
                ViewUniformBufferBinding, TLASBinding, OutImageBinding,
                InImageBinding, GBufferBinding,
                // InitialSampleBufferBinding,
                DiffuseHitDisBinding, SpecularHitDisBinding
        };

        VkDescriptorSetLayoutCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        CreateInfo.bindingCount = Bindings.size();
        CreateInfo.pBindings = Bindings.data();
        CreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;

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

    void VulkanRayTracingApp::UpdateRTDescriptorSets() {
        std::array<VkWriteDescriptorSet, 7> DescriptorWrites{};
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo BufferInfo{};
            BufferInfo.buffer = m_ViewUniformBuffers[i]->GetHandle();
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
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR
            };
            ASInfo.accelerationStructureCount = 1;
            auto TLASHandle = m_RTScene->GetAccelBuilder()->GetTLAS();
            ASInfo.pAccelerationStructures = &TLASHandle;
            DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[1].dstSet = m_RTDescriptorSets[i];
            DescriptorWrites[1].dstBinding = 1;
            DescriptorWrites[1].dstArrayElement = 0;
            DescriptorWrites[1].descriptorCount = 1;
            DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            DescriptorWrites[1].pNext = &ASInfo;

            VkDescriptorImageInfo OutImageInfo{};
            OutImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            OutImageInfo.imageView = m_globalTexture->GetGlobalTexture(TextureType::SceneColor, i)->CreateSRV();
            DescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[2].dstSet = m_RTDescriptorSets[i];
            DescriptorWrites[2].dstBinding = 2;
            DescriptorWrites[2].dstArrayElement = 0;
            DescriptorWrites[2].descriptorCount = 1;
            DescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            DescriptorWrites[2].pImageInfo = &OutImageInfo;

            VkDescriptorImageInfo InImageInfo{};
            InImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            InImageInfo.imageView = m_lastFrameViewportImage->CreateSRV();
            DescriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[3].dstSet = m_RTDescriptorSets[i];
            DescriptorWrites[3].dstBinding = 3;
            DescriptorWrites[3].dstArrayElement = 0;
            DescriptorWrites[3].descriptorCount = 1;
            DescriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            DescriptorWrites[3].pImageInfo = &InImageInfo;

            std::vector<VkDescriptorImageInfo> GBufferInfos(3);
            for (int index = 0; index < GBufferInfos.size(); index++) {
                GBufferInfos[index].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                GBufferInfos[index].sampler = Sampler::GetDefaultSample()->GetHandle();
            }
            GBufferInfos[0].imageView = GetGlobalTexture(TextureType::Albedo_Metallic, i)->CreateSRV();
            GBufferInfos[1].imageView = GetGlobalTexture(TextureType::Normal_Roughness, i)->CreateSRV();
            GBufferInfos[2].imageView = GetGlobalTexture(TextureType::WorldPosition, i)->CreateSRV();
            DescriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[4].dstSet = m_RTDescriptorSets[i];
            DescriptorWrites[4].dstBinding = 4;
            DescriptorWrites[4].dstArrayElement = 0;
            DescriptorWrites[4].descriptorCount = GBufferInfos.size();
            DescriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            DescriptorWrites[4].pImageInfo = GBufferInfos.data();

            // VkDescriptorBufferInfo InitialSampleBufferInfo{};
            // InitialSampleBufferInfo.buffer = m_restirResource->GetInitialSampleBuffer(
                    // i)->GetHandle();
            // InitialSampleBufferInfo.offset = 0;
            // InitialSampleBufferInfo.range = VK_WHOLE_SIZE;
            // DescriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            // DescriptorWrites[5].dstSet = m_RTDescriptorSets[i];
            // DescriptorWrites[5].dstBinding = 5;
            // DescriptorWrites[5].dstArrayElement = 0;
            // DescriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            // DescriptorWrites[5].descriptorCount = 1;
            // DescriptorWrites[5].pBufferInfo = &InitialSampleBufferInfo;

            VkDescriptorImageInfo DiffuseHitDisBinding{};
            DiffuseHitDisBinding.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            DiffuseHitDisBinding.imageView = m_globalTexture->GetGlobalTexture(TextureType::Diffuse_Radiance_HitDis, i)->CreateSRV();
            DescriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[5].dstSet = m_RTDescriptorSets[i];
            DescriptorWrites[5].dstBinding = 5;
            DescriptorWrites[5].dstArrayElement = 0;
            DescriptorWrites[5].descriptorCount = 1;
            DescriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            DescriptorWrites[5].pImageInfo = &DiffuseHitDisBinding;

            VkDescriptorImageInfo SpecularHitDisBinding{};
            SpecularHitDisBinding.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            SpecularHitDisBinding.imageView = m_globalTexture->GetGlobalTexture(TextureType::Specular_Radiance_HitDis, i)->CreateSRV();
            DescriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[6].dstSet = m_RTDescriptorSets[i];
            DescriptorWrites[6].dstBinding = 6;
            DescriptorWrites[6].dstArrayElement = 0;
            DescriptorWrites[6].descriptorCount = 1;
            DescriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            DescriptorWrites[6].pImageInfo = &SpecularHitDisBinding;

            vkUpdateDescriptorSets(m_device, DescriptorWrites.size(), DescriptorWrites.data(),
                                   0, nullptr);
        }
    }

    void VulkanRayTracingApp::CreateRTPipelineLayout() {
        VkPushConstantRange RenderOptionRange{};
        RenderOptionRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                                       VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                                       VK_SHADER_STAGE_MISS_BIT_KHR;
        RenderOptionRange.offset = 0;
        RenderOptionRange.size = sizeof(PathTracingOptions);

        VkPipelineLayoutCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        std::array<VkDescriptorSetLayout, 2> Layouts = {
                m_RTDescriptorSetLayout,
                m_RTScene->GetSceneDescriptorSetLayout()
        };
        CreateInfo.setLayoutCount = Layouts.size();
        CreateInfo.pSetLayouts = Layouts.data();
        CreateInfo.pushConstantRangeCount = 1;
        CreateInfo.pPushConstantRanges = &RenderOptionRange;

        VK_CHECK(vkCreatePipelineLayout(m_device, &CreateInfo, nullptr, &m_RTPipelineLayout));
    }

    void VulkanRayTracingApp::CreateRTPipeline() {
        HLSLCompiler::CompileShader("RayTracing/RayGen.hlsl", "main", ShaderType::RayGen,
                                    "RayTracing/RayGen");
        HLSLCompiler::CompileShader("RayTracing/Miss.hlsl", "main", ShaderType::Miss,
                                    "RayTracing/Miss");
        HLSLCompiler::CompileShader("RayTracing/ClosestHit.hlsl", "main",
                                    ShaderType::ClosestHit,
                                    "RayTracing/ClosestHit");

        // Create Shader Modules
        ShaderBase RayGenShader(ShaderType::RayGen, "../../shader/HLSL/RayTracing/RayGen.spv",
                                "main");
        ShaderBase MissShader(ShaderType::Miss, "../../shader/HLSL/RayTracing/Miss.spv",
                              "main");
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
        ShaderGroupCreateInfo.sType =
                VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
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

        auto CreateRTPipelineFunc = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
                vkGetDeviceProcAddr(
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
        auto Func = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
                vkGetDeviceProcAddr(
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
            memcpy(reinterpret_cast<void *>(pData),
                   Handles.data() + HandleIndex++ * m_RTProps.shaderGroupHandleSize,
                   m_RTProps.shaderGroupHandleSize);
            pData += m_missRegion.stride;
        }
        for (int i = 0; i < HitCount; i++) {
            memcpy(reinterpret_cast<void *>(pData),
                   Handles.data() + HandleIndex++ * m_RTProps.shaderGroupHandleSize,
                   m_RTProps.shaderGroupHandleSize);
            pData += m_hitRegion.stride;
        }
        vkUnmapMemory(m_device, m_RTSBTBuffer->GetMemoryHandle());
    }

    void VulkanRayTracingApp::InitScene() {
        m_RTScene = new Scene();

        // TODO: Choose a default sky texture
        m_RTScene->CreateSkyTexture("../../asset/env/kloofendal_48d_partly_cloudy_puresky_4k.hdr");

        // m_RTScene->AddModel("../../asset/bistro/BistroInterior.fbx");
        // m_RTScene->AddModel("../../asset/bistro/BistroExterior.fbx");
        // m_RTScene->AddModel("../../asset/nezha.gltf");
        m_RTScene->AddModel("../../asset/cornell_box_glossy/cornell_box.gltf");
        // m_RTScene->AddModel("../../asset/catedral-de-chihuahua/source/Catedral_Chihuahua_FINAL.fbx");
        // m_RTScene->AddModel("../../asset/sponza_fbx/sponza.fbx");
        // m_RTScene->AddModel("../../asset/test_material/test_material.glb");

//        m_RTScene->AddModel("../../asset/dragon/Dragon_Baked_Actions.obj");
//        m_RTScene->AddModel("../../asset/house_with_tree/house_with_tree.obj");
//        m_RTScene->AddModel("../../asset/house_with_tree/house_with_tree_glossy.obj");
//        m_RTScene->AddModel("../../asset/cornell_box/cornell_box.obj");
//        m_RTScene->AddModel("../../asset/cornell_box_glossy/cornell_box.obj");
        m_RTScene->AddLight(
                LightType::Directional,
                glm::normalize(glm::vec3(-.41f, -.91f, -0.09f)),
                glm::vec3(.5f, .5f, .5f),
                0.f,
                glm::vec3(1.f, 1.f, 1.f),
                3.f,
                0.035f,
                0.f,
                0.f
        );
//        m_RTScene->AddLight(
//                glm::vec3(0.f, 0.f, 0.f),
//                LightType::Point,
//                glm::vec3(1.f, 2.f, 2.f),
//                .1f,  // Physical Radius
//                glm::vec3(1.f, 1.f, 1.f),
//                10.f,  // Intensity
//                5.f,  // Range
//                0.f,
//                0.f
//        );

//        m_RTScene->AddLight(
//                glm::vec3(0.f, 0.f, 0.f),
//                LightType::Point,
//                glm::vec3(0.f, 1.f, 2.5f),
//                0.f,  // Ideal Point Light
//                glm::vec3(1.f, 1.f, 1.f),
//                10.f,  // Intensity
//                5.f,
//                0.f,
//                0.f
//        );

        m_RTScene->FinalizeScene();
    }

    void VulkanRayTracingApp::CreateViewportImages() {
        m_globalTexture->CreateGlobalTexture(TextureType::SceneColor,
                                             m_viewportSize.x, m_viewportSize.y,
                                             VK_FORMAT_R32G32B32A32_SFLOAT,
                                             VK_IMAGE_USAGE_STORAGE_BIT |
                                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                             VK_IMAGE_USAGE_SAMPLED_BIT,
                                             VK_IMAGE_LAYOUT_GENERAL);
        m_globalTexture->CreateGlobalTexture(TextureType::Diffuse_Radiance_HitDis,
                                             m_viewportSize.x, m_viewportSize.y,
                                             VK_FORMAT_R16G16B16A16_SFLOAT,
                                             VK_IMAGE_USAGE_STORAGE_BIT |
                                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                             VK_IMAGE_USAGE_SAMPLED_BIT,
                                             VK_IMAGE_LAYOUT_GENERAL);
        m_globalTexture->CreateGlobalTexture(TextureType::Specular_Radiance_HitDis,
                                             m_viewportSize.x, m_viewportSize.y,
                                             VK_FORMAT_R16G16B16A16_SFLOAT,
                                             VK_IMAGE_USAGE_STORAGE_BIT |
                                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                             VK_IMAGE_USAGE_SAMPLED_BIT,
                                             VK_IMAGE_LAYOUT_GENERAL);

        m_viewportImageDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        m_currentViewportImageSize = m_viewportSize;

        auto CommandBuffer = RHI::BeginIntermediateCommandBuffer(QueueType::Compute);
        RHI::TextureTransitionInput SrcInput{}, DstInput{};
        m_lastFrameViewportImage = new Texture2D(m_viewportSize.x,
                                                 m_viewportSize.y,
                                                 VK_FORMAT_R32G32B32A32_SFLOAT,
                                                 VK_IMAGE_USAGE_STORAGE_BIT |
                                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                 VK_IMAGE_USAGE_SAMPLED_BIT,
                                                 VK_IMAGE_LAYOUT_UNDEFINED);
        SrcInput.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
        SrcInput.AccessMask = 0;
        SrcInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
        DstInput.Layout = VK_IMAGE_LAYOUT_GENERAL;
        DstInput.AccessMask = 0;
        DstInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
        RHI::TransitionTextureLayout(CommandBuffer, m_lastFrameViewportImage->GetHandle(), 1,
                                     SrcInput, DstInput);
        RHI::SubmitIntermediateCommandBuffer(CommandBuffer, QueueType::Compute);

        m_viewportImageDescriptorSets[0] = ImGui_ImplVulkan_AddTexture(
                Sampler::GetDefaultSample()->GetHandle(),
                GetGlobalTexture(TextureType::SceneColor, 0)->CreateSRV(),
                VK_IMAGE_LAYOUT_GENERAL);
        m_viewportImageDescriptorSets[1] = ImGui_ImplVulkan_AddTexture(
                Sampler::GetDefaultSample()->GetHandle(),
                GetGlobalTexture(TextureType::SceneColor, 1)->CreateSRV(),
                VK_IMAGE_LAYOUT_GENERAL);
    }

    void VulkanRayTracingApp::ResizeViewportImages() {
        if (m_viewportSize == m_currentViewportImageSize) {
            return;
        }

        m_currentViewportImageSize = m_viewportSize;
        m_currentDelayFrames = s_numFramesToDelay;
        vkDeviceWaitIdle(m_device);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            ImGui_ImplVulkan_RemoveTexture(m_viewportImageDescriptorSets[i]);
        }
        m_globalTexture->ReleaseGlobalTexture({TextureType::SceneColor,
                                               TextureType::Diffuse_Radiance_HitDis,
                                               TextureType::Specular_Radiance_HitDis});
        delete m_lastFrameViewportImage;

        CreateViewportImages();
        m_camera->OnResize(m_viewportSize.x, m_viewportSize.y);

        m_gbufferPass->OnResize(m_viewportSize.x, m_viewportSize.y);
        m_accumulationPass->OnResize(m_viewportSize.x, m_viewportSize.y);
        m_accumulationPass->OnResize(m_viewportSize.x, m_viewportSize.y);
        m_bloomPass->OnResize(m_viewportSize.x, m_viewportSize.y);
        // m_restirResource->OnResize(m_viewportSize.x, m_viewportSize.y);

        UpdateRTDescriptorSets();
        m_toneMappingPass->UpdateDescriptorSets();
    }

    void VulkanRayTracingApp::OnWindowResize() {
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
        m_imguiInfrastructure->RecreateFrameBuffer();

        ResizeViewportImages();
        ResetFrameNum();
    }

    void VulkanRayTracingApp::ReloadScene(const std::filesystem::path &Path) {
        vkDeviceWaitIdle(m_device);
        // Release Resources
        vkDestroyDescriptorSetLayout(m_device, m_RTDescriptorSetLayout, nullptr);
        vkFreeDescriptorSets(m_device, m_descriptorPool, m_RTDescriptorSets.size(),
                             m_RTDescriptorSets.data());
        vkDestroyPipelineLayout(m_device, m_RTPipelineLayout, nullptr);
        vkDestroyPipeline(m_device, m_RTPipeline, nullptr);
        delete m_RTSBTBuffer;

        m_gbufferPass->Release();
        m_accumulationPass->Release();
        m_bloomPass->Release();

        m_RTScene->OnRecreate();
        m_RTScene->AddModel(Path);
        m_RTScene->FinalizeScene();

        CreateRTDescriptorSets();
        CreateRTPipelineLayout();
        CreateRTPipeline();
        CreateRTSBT();

        m_gbufferPass->CreateSets();
        m_gbufferPass->CreatePipeline();
        m_accumulationPass->CreateSets();
        m_accumulationPass->CreatePipeline();
        m_bloomPass->CreatePipeline();

        UpdateRTDescriptorSets();
        m_gbufferPass->UpdateSets();
        m_accumulationPass->UpdateSets();
        m_bloomPass->UpdateSets();
    }

    void VulkanRayTracingApp::ReCompileShaders() {
        delete m_RTSBTBuffer;
        vkDestroyPipeline(m_device, m_RTPipeline, nullptr);
        // m_restirResource->DestroyPipeline();
        m_toneMappingPass->DestroyPipeline();

        CreateRTPipeline();
        CreateRTSBT();
        // m_restirResource->CreateTemporalReusePassPipeline();
        // m_restirResource->CreateSpatialReusePassPipeline();
        m_gbufferPass->OnRecompile();
        m_accumulationPass->OnRecompile();
        m_bloomPass->OnRecompile();
        m_toneMappingPass->CreatePipeline();
    }

    void VulkanRayTracingApp::UpdateRenderOptions() {
        m_pathTracingOptions.NumLights = m_RTScene->GetSceneLights().size();
        // m_restirResource->GetTemporalReuseOptions().MaxAccumulatedFrames = m_accumulationPass->GetRenderOptions().MaxAccumulatedFrames;
        // m_restirResource->GetSpatialReuseOptions().MaxAccumulatedFrames = m_accumulationPass->GetRenderOptions().MaxAccumulatedFrames;

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
        ViewUniformBuffer_.Width = m_viewportSize.x;
        ViewUniformBuffer_.Height = m_viewportSize.y;
        m_ViewUniformBuffers[m_imageIndex]->Update(&ViewUniformBuffer_);
    }

} // namespace Shadowy
