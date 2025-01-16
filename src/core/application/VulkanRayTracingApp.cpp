//
// Created by HUSTLX on 2025/1/5.
//

#include "VulkanRayTracingApp.h"

#include <imgui_internal.h>

#include "core/RHI.h"
#include "core/shader/ShaderBase.h"
#include "core/shader_compiler/CompilerHLSL.h"
#include "core/Utils.h"
#include "ImGuiFileDialog.h"


namespace HWPT {
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

        m_camera->SetCameraPosition(glm::vec3(-600, 510, 40));
        m_camera->SetCameraRotation(-0.13, -1.41);
//        m_camera->SetCameraRotation(-glm::radians(10.f), glm::radians(10.f));
    }

    void VulkanRayTracingApp::CleanUp() {
        Sampler::ReleaseSamplers();
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            delete m_viewportImages[i];
        }
        delete m_gBuffer;
        delete m_lastFrameViewportImage;
        delete m_RTScene;
        delete m_RTSBTBuffer;
        delete m_msaaBuffers;
        CleanUpImGui();
        CleanUpSwapChain();

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            delete m_MVPUniformBuffers[i];
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
        for (auto &Operation: m_deferredOperations) {
            Operation();
        }
        m_deferredOperations.clear();

        // Update ViewUniformBuffer
        ViewUniformBuffer ViewUniformBuffer_{};
        ViewUniformBuffer_.ViewTrans = m_camera->GetViewMatrix();
        ViewUniformBuffer_.ProjTrans = m_camera->GetProjMatrix();
        ViewUniformBuffer_.CameraPos = m_camera->GetCameraPos();
        ViewUniformBuffer_.DebugColor = glm::vec3(.5f, .9f, .6f);
        ViewUniformBuffer_.DeltaTime = m_fpsCalculator
                                       ? static_cast<float>(m_fpsCalculator->GetDeltaTime())
                                       : 0.f;
        ViewUniformBuffer_.FrameNum = m_frameNum;
        ViewUniformBuffer_.InvView = glm::transpose(m_camera->GetViewMatrix());
        ViewUniformBuffer_.InvProj = glm::inverse(m_camera->GetProjMatrix());
        m_MVPUniformBuffers[m_imageIndex]->Update(&ViewUniformBuffer_);

        // Wait for last frame render finish
        vkWaitForFences(m_device, 1, &m_graphicsInFlightFences[m_currentFrame], VK_TRUE,
                        UINT64_MAX);
        vkResetFences(m_device, 1, &m_graphicsInFlightFences[m_currentFrame]);

        if (m_frameBufferResized) {
            OnWindowResize();
            m_frameBufferResized = false;
        }

        // Wait for image available
        VkResult Result = vkAcquireNextImageKHR(m_device, m_swapChain.SwapChainHandle, UINT64_MAX,
                                                m_imageAvailableSemaphores[m_currentFrame],
                                                VK_NULL_HANDLE, &m_imageIndex);
        if (Result == VK_ERROR_OUT_OF_DATE_KHR || Result == VK_SUBOPTIMAL_KHR) {
            OnWindowResize();
        } else if (Result != VK_SUCCESS) {
            throw std::runtime_error("Failed to acquire swap chain images");
        }

        auto CommandBuffer = m_graphicsCommandBuffers[m_currentFrame];
        auto CurrentFrameSceneColor = m_swapChain.SwapChainImages[m_imageIndex];
        auto CurrentFrameViewportImage = m_viewportImages[m_imageIndex];
        vkResetCommandBuffer(CommandBuffer, 0);
        VkCommandBufferBeginInfo BeginInfo{};
        BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_CHECK(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

        RHI::TextureTransitionInput SrcInput{}, DstInput{};
        SrcInput.Layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
        SrcInput.AccessMask = 0;
        SrcInput.PipelineStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        DstInput.Layout = VK_IMAGE_LAYOUT_GENERAL;
        DstInput.AccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        DstInput.PipelineStage = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
        RHI::TransitionTextureLayout(CommandBuffer, CurrentFrameViewportImage->GetHandle(), 1,
                                     SrcInput, DstInput);

        RHI::TransitionTextureLayout(CommandBuffer,
                                     m_gBuffer->GetGBufferAlbedo(m_imageIndex)->GetHandle(),
                                     1, SrcInput, DstInput);
        RHI::TransitionTextureLayout(CommandBuffer,
                                     m_gBuffer->GetGBufferNormal(m_imageIndex)->GetHandle(),
                                     1, SrcInput, DstInput);
        RHI::TransitionTextureLayout(CommandBuffer,
                                     m_gBuffer->GetGBufferPos(m_imageIndex)->GetHandle(),
                                     1, SrcInput, DstInput);
        RHI::TransitionTextureLayout(CommandBuffer,
                                     m_gBuffer->GetGBufferDepth(m_imageIndex)->GetHandle(),
                                     1, SrcInput, DstInput);

        // TODO: Add RayTracing Code Here
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
        m_renderOptions.ShouldReAccumulate = m_camera->IsMoving();
        m_renderOptions.NumLights = m_RTScene->GetSceneLights().size();
        vkCmdPushConstants(CommandBuffer, m_RTPipelineLayout,
                           VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                           VK_SHADER_STAGE_MISS_BIT_KHR,
                           0, sizeof(RenderOptions), &m_renderOptions);
        auto RayTraceFunc = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(m_device,
                                                                                        "vkCmdTraceRaysKHR"));
        RayTraceFunc(CommandBuffer, &m_rayGenRegion, &m_missRegion, &m_hitRegion,
                     &m_callRegion, m_viewportSize.x, m_viewportSize.y, 1);

        /*
         * Transition Current ColorAttach to VK_IMAGE_LAYOUT_VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
         * Transition m_lastFrameSceneColor to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL for CopyImage
         */
        // RHI::TextureTransitionInput SrcInput{}, DstInput{};
        SrcInput.Layout = VK_IMAGE_LAYOUT_GENERAL;
        SrcInput.AccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        SrcInput.PipelineStage = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
        DstInput.Layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        DstInput.AccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        DstInput.PipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        RHI::TransitionTextureLayout(CommandBuffer, CurrentFrameViewportImage->GetHandle(), 1,
                                     SrcInput, DstInput);
        SrcInput.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        SrcInput.AccessMask = 0;
        SrcInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
        DstInput.Layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        DstInput.AccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        DstInput.PipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        RHI::TransitionTextureLayout(CommandBuffer, m_lastFrameViewportImage->GetHandle(), 1,
                                     SrcInput,
                                     DstInput);
        VkImageCopy ImageCopy{};
        ImageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ImageCopy.srcSubresource.baseArrayLayer = 0;
        ImageCopy.srcSubresource.layerCount = 1;
        ImageCopy.srcSubresource.mipLevel = 0;
        ImageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ImageCopy.dstSubresource.baseArrayLayer = 0;
        ImageCopy.dstSubresource.layerCount = 1;
        ImageCopy.dstSubresource.mipLevel = 0;
        VkOffset3D Offset{0, 0, 0};
        ImageCopy.srcOffset = Offset;
        ImageCopy.dstOffset = Offset;
        VkExtent3D Extent3D{
                static_cast<uint>(m_viewportSize.x), static_cast<uint>(m_viewportSize.y), 1
        };
        ImageCopy.extent = Extent3D;
        vkCmdCopyImage(
                CommandBuffer,
                CurrentFrameViewportImage->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                m_lastFrameViewportImage->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &ImageCopy
        );

        /*
         * Transition Current ColorAttach to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for ColorAttach and Present
         * Transition m_lastFrameSceneColor to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
         */
        SrcInput.Layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        SrcInput.AccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        SrcInput.PipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        DstInput.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        DstInput.AccessMask = VK_ACCESS_SHADER_READ_BIT;
        DstInput.PipelineStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        RHI::TransitionTextureLayout(CommandBuffer, CurrentFrameViewportImage->GetHandle(), 1,
                                     SrcInput, DstInput);
        SrcInput.Layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        SrcInput.AccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        SrcInput.PipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        DstInput.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        DstInput.AccessMask = VK_ACCESS_SHADER_READ_BIT;
        DstInput.PipelineStage = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
        RHI::TransitionTextureLayout(CommandBuffer, m_lastFrameViewportImage->GetHandle(),
                                     1, SrcInput, DstInput);

        SrcInput.Layout = VK_IMAGE_LAYOUT_GENERAL;
        SrcInput.AccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        SrcInput.PipelineStage = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
        DstInput.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        DstInput.AccessMask = VK_ACCESS_SHADER_READ_BIT;
        DstInput.PipelineStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        RHI::TransitionTextureLayout(CommandBuffer,
                                     m_gBuffer->GetGBufferAlbedo(m_imageIndex)->GetHandle(),
                                     1, SrcInput, DstInput);
        RHI::TransitionTextureLayout(CommandBuffer,
                                     m_gBuffer->GetGBufferNormal(m_imageIndex)->GetHandle(),
                                     1, SrcInput, DstInput);
        RHI::TransitionTextureLayout(CommandBuffer,
                                     m_gBuffer->GetGBufferPos(m_imageIndex)->GetHandle(),
                                     1, SrcInput, DstInput);
        RHI::TransitionTextureLayout(CommandBuffer,
                                     m_gBuffer->GetGBufferDepth(m_imageIndex)->GetHandle(),
                                     1, SrcInput, DstInput);

        m_imguiInfrastructure->BeginImGui();
        DrawImGuiFrame();
        m_imguiInfrastructure->EndImGui(CommandBuffer);

        vkEndCommandBuffer(CommandBuffer);
        // Submit Commands
        VkSubmitInfo SubmitInfo{};
        SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        std::array<VkSemaphore, 1> WaitSemaphores = {
                m_imageAvailableSemaphores[m_currentFrame]
        };
        std::array<VkPipelineStageFlags, 1> WaitStages = {
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };
        SubmitInfo.pWaitDstStageMask = WaitStages.data();
        SubmitInfo.waitSemaphoreCount = WaitSemaphores.size();
        SubmitInfo.pWaitSemaphores = WaitSemaphores.data();

        SubmitInfo.commandBufferCount = 1;
        SubmitInfo.pCommandBuffers = &CommandBuffer;
        SubmitInfo.signalSemaphoreCount = 1;
        SubmitInfo.pSignalSemaphores = &m_renderFinishedSemaphores[m_currentFrame];
        VK_CHECK(vkQueueSubmit(m_commandPool->GetGraphicsQueue(), 1, &SubmitInfo,
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

        CreateViewportImages();
        CreateGBuffer();

        InitScene();
        CreateRTDescriptorSets();
        BindRTDescriptorSets();
        CreateRTPipelineLayout();
        CreateRTPipeline();
        CreateRTSBT();
    }

    void VulkanRayTracingApp::DrawImGuiFrame() {
        {
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
                m_deferredOperations.emplace_back([this](){
                    ResizeViewportImages();
                    m_gBuffer->OnResize(m_viewportSize);
                });
            }

            if (IsWindowHovered) {
                m_camera->Tick(m_fpsCalculator->GetDeltaTime());
            }

            ImGui::Image(m_viewportImageDescriptorSets[m_imageIndex],
                         {m_viewportSize.x, m_viewportSize.y});

            ImGuiInfrastructure::End();
        }

        static bool ShouldReAccumulate = false;
        ShouldReAccumulate = false;
        static bool ShowGBuffer = false;
        {
            if (ShowGBuffer) {
                ImGui::Begin("GBuffer");
                ImVec2 GBufferWindowSize = ImGui::GetContentRegionAvail();
                float ViewPortAspectRatio = m_viewportSize.x / m_viewportSize.y;
                float GBufferAspectRatio = GBufferWindowSize.x / GBufferWindowSize.y;

                ImVec2 ImageRegion{};
                // GBuffer Window Width is lager, align with WindowHeight
                if (GBufferAspectRatio >= ViewPortAspectRatio) {
                    ImageRegion = {
                            GBufferWindowSize.y * ViewPortAspectRatio, GBufferWindowSize.y
                    };
                } else // GBuffer Window Height is lager, align with WindowWidth
                {
                    ImageRegion = {
                            GBufferWindowSize.x, GBufferWindowSize.x / ViewPortAspectRatio
                    };
                }
                ImVec2 SizePerGBufferImage = {
                        ImageRegion.x / 2.05f, ImageRegion.y / 2.05f
                };

                ImGui::Image(m_gBuffer->GetGBufferAlbedoDescriptorSet(m_imageIndex),
                             SizePerGBufferImage);
                ImGui::SameLine();
                ImGui::Image(m_gBuffer->GetGBufferNormalDescriptorSet(m_imageIndex),
                             SizePerGBufferImage);
                ImGui::Image(m_gBuffer->GetGBufferPosDescriptorSet(m_imageIndex),
                             SizePerGBufferImage);
                ImGui::SameLine();
                ImGui::Image(m_gBuffer->GetGBufferDepthDescriptorSet(m_imageIndex),
                             SizePerGBufferImage);

                ImGui::End();
            }
        }

        {
            ImGuiInfrastructure::Begin("Settings");
            ImGui::Text("FPS: %d", m_fpsCalculator->GetFPS());
            ImGui::Text("Accumulated Frames: %d", m_frameNum);
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

            ImGui::NewLine();
            ImGui::Separator();
            ImGui::NewLine();
            ImGui::Text("Render Options");
            ImGui::Checkbox("Show GBuffer", &ShowGBuffer);
            ShouldReAccumulate |= ImGui::Checkbox("Enable AO",
                                                  reinterpret_cast<bool *>(&m_renderOptions.EnableAO));
            ShouldReAccumulate |= ImGui::InputInt("Num AO Rays", &m_renderOptions.NumAORays, 1.f);
            ShouldReAccumulate |= ImGui::InputFloat("AO Ray Length", &m_renderOptions.AORayLength,
                                                    .5f, 1.f, "%.1f");

            ImGui::NewLine();
            ImGui::Separator();
            ImGui::NewLine();
            ImGui::Text("Path Tracing Options");
            ShouldReAccumulate |= ImGui::Checkbox("Enable Accumulation",
                                                  reinterpret_cast<bool *>(&m_renderOptions.EnableAccumulation));
            ShouldReAccumulate |= ImGui::InputFloat("Min Ray Bias", &m_renderOptions.RayMinBias,
                                                    1e-3f);
            ShouldReAccumulate |= ImGui::InputInt("Bounce", &m_renderOptions.Bounce, 1.f);
            ShouldReAccumulate |= ImGui::Checkbox("Enable SkyLight",
                                                  reinterpret_cast<bool *>(&m_renderOptions.AccumulateSkyLight));

            // TODO: Choose Obj
//            if (ImGui::Button("Open Obj")) {
//                IGFD::FileDialogConfig config;
//                config.path = "../../asset";
//                ImGuiFileDialog::Instance()->OpenDialog("ChooseObj", "Choose File",
//                                                        ".obj", config);
//            }
            if (ImGui::Button("Open Sky Texture")) {
                IGFD::FileDialogConfig config;
                config.path = "../../asset/env";
                ImGuiFileDialog::Instance()->OpenDialog("ChooseSkyTexture", "Choose File",
                                                        ".hdr, .jpg, .png", config);
            }
//            if (ImGuiFileDialog::Instance()->Display("ChooseObj")) { // => will show a dialog
//                if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
//                    std::string FilePathName = ImGuiFileDialog::Instance()->GetFilePathName();
//                    std::string FilePath = ImGuiFileDialog::Instance()->GetCurrentPath();
//                    vkDeviceWaitIdle(m_device);
//                    m_deferredOperations.emplace_back([this, FilePathName]() {
//
//                        m_RTScene->UpdateModelDescDescriptorSets();
//                        ResetFrameNum();
//                    });
//                }
//                ImGuiFileDialog::Instance()->Close();
//            }
            if (ImGuiFileDialog::Instance()->Display("ChooseSkyTexture")) { // => will show a dialog
                if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
                    std::string FilePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                    std::string FilePath = ImGuiFileDialog::Instance()->GetCurrentPath();
                    vkDeviceWaitIdle(m_device);
                    m_deferredOperations.emplace_back([this, FilePathName]() {
                        ResetFrameNum();
                        m_RTScene->CreateSkyTexture(FilePathName);
                        m_RTScene->UpdateModelDescDescriptorSets();
                    });
                }
                ImGuiFileDialog::Instance()->Close();
            }

            if (ShouldReAccumulate) {
                m_deferredOperations.emplace_back([this]() {
                    ResetFrameNum();
                });
            }

//            ImGui::NewLine();
//            ImGui::Separator();
//            ImGui::NewLine();
//            ImGui::Text("Window Options");
//            static bool BorderlessWindow = false;
//            if (ImGui::Checkbox("Borderless Window", &BorderlessWindow))
//            {
//                glfwSetWindowAttrib(m_window, GLFW_DECORATED, !BorderlessWindow);
//            }

//            if (!FullScreen)
//            {
//                glfwGetWindowSize(m_window, &WindowWidth, &WindowHeight);
//                glfwGetWindowPos(m_window, &WindowPosX, &WindowPosY);
//            }
//            if (ImGui::Checkbox("Full Screen", &FullScreen))
//            {
//                if (FullScreen)
//                {
//                    const GLFWvidmode* Mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
//                    glfwSetWindowPos(m_window, 0, 0);
//                    glfwSetWindowSize(m_window, Mode->width, Mode->height);
//                }
//                else
//                {
//                    glfwSetWindowPos(m_window, WindowPosX, WindowPosY);
//                    glfwSetWindowSize(m_window, WindowWidth, WindowHeight);
//                }
//            }

            ImGuiInfrastructure::End();
        }

        {
            ImGuiInfrastructure::Begin("Scene Info");
            ImGui::Text("Camera Info");
            auto CameraPos = m_camera->GetCameraPos();
            ImGui::Text("Camera Position (%.2f, %.2f, %.2f)", CameraPos.x, CameraPos.y,
                        CameraPos.z);
            auto PerspCamera = std::dynamic_pointer_cast<PerspectiveCamera>(m_camera);
            if (ImGui::SliderFloat("FOV", &PerspCamera->GetFOV(), 5.f, 120.f, "%.0f")) {
                m_deferredOperations.emplace_back([this, PerspCamera]() {
                    ResetFrameNum();
                    PerspCamera->UpdateProjMatrix();
                });
            }
            ImGui::Text("Pitch %.2f:  Yaw %.2f", m_camera->GetPitch(), m_camera->GetYaw());
            ImGui::SliderFloat("Move Speed", &m_camera->GetCameraMoveSpeed(), .01f, 100.f, "%.1f");
            ImGui::SliderFloat("Rotate Speed", &m_camera->GetCameraRotateSpeed(), .01f, 5.f,
                               "%.1f");
            ImGui::SliderFloat("Scroll Speed", &m_camera->GetCameraScrollSpeed(), .01f, 100.f,
                               "%.1f");

            ImGui::NewLine();
            ImGui::Separator();
            ImGui::NewLine();

            ImGui::Text("Model Info");

            ImGui::NewLine();
            ImGui::Separator();
            ImGui::NewLine();

            ImGui::Text("Light Info");

            ImGuiInfrastructure::End();
        }
    }

    void VulkanRayTracingApp::CreateRTDescriptorSets() {
        VkDescriptorSetLayoutBinding ViewUniformBufferBinding{};
        ViewUniformBufferBinding.binding = 0;
        ViewUniformBufferBinding.descriptorCount = 1;
        ViewUniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ViewUniformBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

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
        InImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        InImageBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        VkDescriptorSetLayoutBinding GBufferBinding{};
        GBufferBinding.binding = 4;
        GBufferBinding.descriptorCount = 4;
        GBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        GBufferBinding.stageFlags =
                VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        std::array<VkDescriptorSetLayoutBinding, 5> Bindings = {
                ViewUniformBufferBinding, TLASBinding, OutImageBinding, InImageBinding,
                GBufferBinding
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

        // Create Model Desc DescriptorSets
        m_RTScene->CreateModelDescDescriptorSet();
    }

    void VulkanRayTracingApp::BindRTDescriptorSets() {
        std::array<VkWriteDescriptorSet, 5> DescriptorWrites{};
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
            OutImageInfo.imageView = m_viewportImages[i]->CreateSRV();
            DescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[2].dstSet = m_RTDescriptorSets[i];
            DescriptorWrites[2].dstBinding = 2;
            DescriptorWrites[2].dstArrayElement = 0;
            DescriptorWrites[2].descriptorCount = 1;
            DescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            DescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            DescriptorWrites[2].pImageInfo = &OutImageInfo;

            VkDescriptorImageInfo InImageInfo{};
            InImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            InImageInfo.imageView = m_lastFrameViewportImage->CreateSRV();
            DescriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[3].dstSet = m_RTDescriptorSets[i];
            DescriptorWrites[3].dstBinding = 3;
            DescriptorWrites[3].dstArrayElement = 0;
            DescriptorWrites[3].descriptorCount = 1;
            DescriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            DescriptorWrites[3].pImageInfo = &InImageInfo;

            std::vector<VkDescriptorImageInfo> GBufferInfos(4);
            for (int index = 0; index < GBufferInfos.size(); index++) {
                GBufferInfos[index].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                GBufferInfos[index].sampler = Sampler::GetDefaultSample()->GetHandle();
            }
            GBufferInfos[0].imageView = m_gBuffer->GetGBufferAlbedo(i)->CreateSRV();
            GBufferInfos[1].imageView = m_gBuffer->GetGBufferNormal(i)->CreateSRV();
            GBufferInfos[2].imageView = m_gBuffer->GetGBufferPos(i)->CreateSRV();
            GBufferInfos[3].imageView = m_gBuffer->GetGBufferDepth(i)->CreateSRV();
            DescriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[4].dstSet = m_RTDescriptorSets[i];
            DescriptorWrites[4].dstBinding = 4;
            DescriptorWrites[4].dstArrayElement = 0;
            DescriptorWrites[4].descriptorCount = GBufferInfos.size();
            DescriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            DescriptorWrites[4].pImageInfo = GBufferInfos.data();

            vkUpdateDescriptorSets(m_device, DescriptorWrites.size(), DescriptorWrites.data(),
                                   0,
                                   nullptr);
        }

        // Update ModelDesc DescriptorSets
        m_RTScene->UpdateModelDescDescriptorSets();
    }

    void VulkanRayTracingApp::CreateRTPipelineLayout() {
        VkPushConstantRange RenderOptionRange{};
        RenderOptionRange.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                                       VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                                       VK_SHADER_STAGE_MISS_BIT_KHR;
        RenderOptionRange.offset = 0;
        RenderOptionRange.size = sizeof(RenderOptions);

        VkPipelineLayoutCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        std::array<VkDescriptorSetLayout, 2> Layouts = {
                m_RTDescriptorSetLayout,
                m_RTScene->GetModelDescDescriptorSetLayout()
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
        m_RTScene->CreateSkyTexture("../../asset/env/wildflower_field_4k.hdr");
        m_RTScene->AddModel("../../asset/sponza/sponza.obj");
//        m_RTScene->AddModel("../../asset/house_with_tree/house_with_tree.obj");
//        m_RTScene->AddModel("../../asset/cornell_box/cornell_box.obj");
        m_RTScene->AddLight(
                glm::normalize(glm::vec3(.0f, -1.f, -.1f)),
                LightType::Directional,
                glm::vec3(.5f, .5f, .5f),
                0.f,
                glm::vec3(1.f, 1.f, 1.f),
                2.f * 3.1415926f,
                0.f,
                0.f,
                0.f
        );
//        m_RTScene->AddLight(
//                glm::vec3(0.f, 0.f, 0.f),
//                LightType::Point,
//                glm::vec3(1.f, 2.f, 2.f),
//                10.f,  // Radius
//                glm::vec3(1.f, 1.f, 1.f),
//                2.f,  // Intensity
//                0.f,
//                0.f,
//                0.f
//        );

        m_RTScene->FinalizeScene();
    }

    void VulkanRayTracingApp::CreateViewportImages() {
        if (m_viewportImages.size() != MAX_FRAMES_IN_FLIGHT) {
            m_viewportImages.resize(MAX_FRAMES_IN_FLIGHT);
            m_viewportImageDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        }
        auto CommandBuffer = RHI::BeginIntermediateCommandBuffer();
        RHI::TextureTransitionInput SrcInput{}, DstInput{};
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_viewportImages[i] = new Texture2D(m_viewportSize.x,
                                                m_viewportSize.y,
                                                TextureFormat::RGBA,
                                                TextureUsage::UAV);
            SrcInput.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
            SrcInput.AccessMask = 0;
            SrcInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
            DstInput.Layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
            DstInput.AccessMask = 0;
            DstInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
            RHI::TransitionTextureLayout(CommandBuffer, m_viewportImages[i]->GetHandle(), 1,
                                         SrcInput, DstInput);

            m_viewportImageDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(
                    Sampler::GetDefaultSample()->GetHandle(),
                    m_viewportImages[i]->
                            CreateSRV(),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        m_lastFrameViewportImage = new Texture2D(m_viewportSize.x,
                                                 m_viewportSize.y,
                                                 TextureFormat::RGBA,
                                                 TextureUsage::SRV);
        SrcInput.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
        SrcInput.AccessMask = 0;
        SrcInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
        DstInput.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        DstInput.AccessMask = 0;
        DstInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
        RHI::TransitionTextureLayout(CommandBuffer, m_lastFrameViewportImage->GetHandle(), 1,
                                     SrcInput, DstInput);
        RHI::SubmitIntermediateCommandBuffer(CommandBuffer);
    }

    void VulkanRayTracingApp::ResizeViewportImages() {
        vkDeviceWaitIdle(m_device);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            ImGui_ImplVulkan_RemoveTexture(m_viewportImageDescriptorSets[i]);
            delete m_viewportImages[i];
        }
        delete m_lastFrameViewportImage;
        CreateViewportImages();
        m_gBuffer->OnResize(m_viewportSize.x, m_viewportSize.y);
        BindRTDescriptorSets();
        ResetFrameNum();

        m_camera->OnWindowResize(m_viewportSize.x, m_viewportSize.y);
    }

    void VulkanRayTracingApp::CreateGBuffer() {
        m_gBuffer = new GBuffer(MAX_FRAMES_IN_FLIGHT, m_viewportSize);
    }

    void VulkanRayTracingApp::OnWindowResize() {
        VulkanBackendApp::OnWindowResize();
        BindRTDescriptorSets();
        ResetFrameNum();
    }

} // namespace HWPT
