//
// Created by HUSTLX on 2024/10/7.
//

#ifndef HARDWAREPATHTRACER_INDEXBUFFER_H
#define HARDWAREPATHTRACER_INDEXBUFFER_H

#include "core/Core.h"


namespace HWPT {
    class IndexBuffer {
    public:
        IndexBuffer(uint IndexCount, const void *Data);

        IndexBuffer(uint IndexCount, const uint *Data);

        ~IndexBuffer();

        void Bind(VkCommandBuffer CommandBuffer) const;

        auto GetHandle() -> VkBuffer& {
            return m_indexBuffer;
        }

        [[nodiscard]] auto GetIndexCount() const -> uint {
            return m_indexCount;
        }

    private:
        VkBuffer m_indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;
        uint m_indexCount = 0;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_INDEXBUFFER_H
