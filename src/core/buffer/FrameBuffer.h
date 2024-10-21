//
// Created by HUSTLX on 2024/10/21.
//

#ifndef HARDWAREPATHTRACER_FRAMEBUFFER_H
#define HARDWAREPATHTRACER_FRAMEBUFFER_H

#include "core/Core.h"
#include "core/renderGraph/RasterPass.h"


namespace HWPT {
    class FrameBuffer {
    public:
        FrameBuffer(uint Width, uint Height, std::initializer_list<VkImageView>& AttachmentViews, RasterPass* BasePass);

        ~FrameBuffer();

        void Create(std::initializer_list<VkImageView> &AttachmentViews);

        void Destroy();

        void OnResize(uint Width, uint Height, std::initializer_list<VkImageView>& AttachmentViews);

        auto GetHandle() -> VkFramebuffer& {
            return m_frameBuffer;
        }

        [[nodiscard]] auto GetHandle() const -> const VkFramebuffer& {
            return m_frameBuffer;
        }

    private:
        VkFramebuffer m_frameBuffer = VK_NULL_HANDLE;
        RasterPass* m_basePass = nullptr;
        uint m_width = 0, m_height = 0;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_FRAMEBUFFER_H
