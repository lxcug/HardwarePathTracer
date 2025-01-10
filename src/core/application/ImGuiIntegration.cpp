//
// Created by HUSTLX on 2024/10/13.
//

#include "ImGuiIntegration.h"
#include "core/application/VulkanBackendApp.h"
#include "core/RHI.h"


namespace HWPT {

    ImGuiInfrastructure::ImGuiInfrastructure(uint NumFrames) : m_numFrames(NumFrames) {
        m_frameBuffers.resize(NumFrames);

        // Create Descriptor Pool
        VkDescriptorPoolSize ImGuiPoolSize{};
        ImGuiPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        ImGuiPoolSize.descriptorCount = MaxNumTextures;
        VkDescriptorPoolCreateInfo ImGuiPoolInfo{};
        ImGuiPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ImGuiPoolInfo.poolSizeCount = 1;
        ImGuiPoolInfo.pPoolSizes = &ImGuiPoolSize;
        ImGuiPoolInfo.maxSets = MaxNumTextures;
        ImGuiPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        VK_CHECK(vkCreateDescriptorPool(GetVKDevice(), &ImGuiPoolInfo, nullptr, &m_descriptorPool));

        // CreateRenderPass
        VkAttachmentDescription ColorAttachment{};
        ColorAttachment.format = VulkanBackendApp::GetApplication()->GetSwapChain().Format;
        ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        ColorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
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
        Dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        Dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        Dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        CreateInfo.attachmentCount = 1;
        CreateInfo.pAttachments = &ColorAttachment;
        CreateInfo.subpassCount = 1;
        CreateInfo.pSubpasses = &SubPass;
        CreateInfo.dependencyCount = 1;
        CreateInfo.pDependencies = &Dependency;

        VK_CHECK(vkCreateRenderPass(GetVKDevice(), &CreateInfo, nullptr, &m_renderPass));

        // CreateFrameBuffers
        SwapChain _SwapChain = VulkanBackendApp::GetApplication()->GetSwapChain();
        for (size_t Index = 0; Index < m_frameBuffers.size(); Index++) {

            std::array<VkImageView, 1> Attachments = {
                    _SwapChain.SwapChainImageViews[Index]
            };

            VkFramebufferCreateInfo FrameBufferCreateInfo{};
            FrameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            FrameBufferCreateInfo.renderPass = m_renderPass;
            FrameBufferCreateInfo.width = _SwapChain.Extent.width;
            FrameBufferCreateInfo.height = _SwapChain.Extent.height;
            FrameBufferCreateInfo.layers = 1;
            FrameBufferCreateInfo.attachmentCount = Attachments.size();
            FrameBufferCreateInfo.pAttachments = Attachments.data();

            VK_CHECK(vkCreateFramebuffer(GetVKDevice(), &FrameBufferCreateInfo, nullptr,
                                         &m_frameBuffers[Index]));
        }
    }

    ImGuiInfrastructure::~ImGuiInfrastructure() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        vkDestroyDescriptorPool(GetVKDevice(), m_descriptorPool, nullptr);
        vkDestroyRenderPass(GetVKDevice(), m_renderPass, nullptr);
        for (int i = 0; i < m_numFrames; i++) {
            vkDestroyFramebuffer(GetVKDevice(), m_frameBuffers[i], nullptr);
        }
    }

    void ImGuiInfrastructure::RecreateFrameBuffer() {
        SwapChain _SwapChain = VulkanBackendApp::GetApplication()->GetSwapChain();
        for (size_t Index = 0; Index < m_frameBuffers.size(); Index++) {
            vkDestroyFramebuffer(GetVKDevice(), m_frameBuffers[Index], nullptr);

            std::array<VkImageView, 1> Attachments = {
                    _SwapChain.SwapChainImageViews[Index]
            };

            VkFramebufferCreateInfo FrameBufferCreateInfo{};
            FrameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            FrameBufferCreateInfo.renderPass = m_renderPass;
            FrameBufferCreateInfo.width = _SwapChain.Extent.width;
            FrameBufferCreateInfo.height = _SwapChain.Extent.height;
            FrameBufferCreateInfo.layers = 1;
            FrameBufferCreateInfo.attachmentCount = Attachments.size();
            FrameBufferCreateInfo.pAttachments = Attachments.data();

            VK_CHECK(vkCreateFramebuffer(GetVKDevice(), &FrameBufferCreateInfo, nullptr,
                                         &m_frameBuffers[Index]));
        }
    }

    void ImGuiInfrastructure::BeginImGui() {
        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplVulkan_NewFrame();
        ImGui::NewFrame();

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoDocking |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus |
                                        ImGuiWindowFlags_NoNavFocus;
        // NOTE: No Background
        window_flags |= ImGuiWindowFlags_NoBackground;

        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
        // all active windows docked into it will lose their parent and become undocked.
        // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
        // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", nullptr, window_flags);
        ImGui::PopStyleVar(3);

        // DockSpace
        ImGuiIO &io = ImGui::GetIO();
        ImGuiStyle &style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 200.f;

        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
//        | ImGuiDockNodeFlags_AutoHideTabBar;
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImVec4 color = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
            ImGui::PushStyleColor(ImGuiCol_WindowBg,
                                  ImVec4(color.x, color.y, color.z, m_imguiUIAlpha));
            ImGuiID dockspace_id = ImGui::GetID("DockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
            ImGui::PopStyleColor();
        }
        style.WindowMinSize.x = minWinSizeX;
        ImGui::End();
    }

    void ImGuiInfrastructure::EndImGui() {
        auto App = VulkanBackendApp::GetApplication();
        auto CommandBuffer = RHI::BeginIntermediateCommandBuffer();

        VkRenderPassBeginInfo RenderPassInfo{};
        RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        RenderPassInfo.renderPass = m_renderPass;
        RenderPassInfo.framebuffer = m_frameBuffers[App->GetImageIndex()];
        RenderPassInfo.renderArea.offset = {0, 0};
        RenderPassInfo.renderArea.extent = App->GetSwapChain().Extent;
        VkClearValue ClearValue = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        RenderPassInfo.clearValueCount = 1;
        RenderPassInfo.pClearValues = &ClearValue;
        vkCmdBeginRenderPass(CommandBuffer, &RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), CommandBuffer);

        vkCmdEndRenderPass(CommandBuffer);
        RHI::SubmitIntermediateCommandBuffer(CommandBuffer);

        // For ImGui MultiView
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(App->GetWindow());
    }

}  // namespace HWPT
