//
// Created by HUSTLX on 2024/10/21.
//

#include "FrameBuffer.h"


namespace HWPT {
    FrameBuffer::FrameBuffer(uint Width, uint Height,
                             std::initializer_list<VkImageView> &AttachmentViews,
                             RasterPass *BasePass)
            : m_width(Width), m_height(Height), m_basePass(BasePass) {
        Create(AttachmentViews);
    }

    FrameBuffer::~FrameBuffer() {
        Destroy();
    }

    void FrameBuffer::OnResize(uint Width, uint Height,
                               std::initializer_list<VkImageView> &AttachmentViews) {
        Destroy();

        m_width = Width;
        m_height = Height;
        Create(AttachmentViews);
    }

    void FrameBuffer::Destroy() {
        vkDestroyFramebuffer(GetVKDevice(), m_frameBuffer, nullptr);
    }

    void FrameBuffer::Create(std::initializer_list<VkImageView> &AttachmentViews) {
        VkFramebufferCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        CreateInfo.renderPass = m_basePass->GetRenderPass();
        CreateInfo.width = m_width;
        CreateInfo.height = m_height;
        CreateInfo.layers = 1;
        CreateInfo.attachmentCount = AttachmentViews.size();
        CreateInfo.pAttachments = data(AttachmentViews);

        VK_CHECK(vkCreateFramebuffer(GetVKDevice(), &CreateInfo, nullptr, &m_frameBuffer));
    }
}  // namespace HWPT
