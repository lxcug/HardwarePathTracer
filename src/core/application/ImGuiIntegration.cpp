//
// Created by HUSTLX on 2024/10/13.
//

#include "ImGuiIntegration.h"
#include "core/application/VulkanBackendApp.h"


namespace HWPT {

    ImGuiInfrastructure::ImGuiInfrastructure(uint NumFrames): m_numFrames(NumFrames) {
//        m_commandBuffers.resize(NumFrames);
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

            VK_CHECK(vkCreateFramebuffer(GetVKDevice(), &FrameBufferCreateInfo, nullptr, &m_frameBuffers[Index]));
        }
    }

    ImGuiInfrastructure::~ImGuiInfrastructure() {
        vkDestroyDescriptorPool(GetVKDevice(), m_descriptorPool, nullptr);
//        vkDestroyCommandPool(GetVKDevice(), m_commandPool, nullptr);
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

            VK_CHECK(vkCreateFramebuffer(GetVKDevice(), &FrameBufferCreateInfo, nullptr, &m_frameBuffers[Index]));
        }
    }

}  // namespace HWPT
