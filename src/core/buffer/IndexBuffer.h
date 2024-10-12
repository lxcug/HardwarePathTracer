//
// Created by HUSTLX on 2024/10/7.
//

#ifndef HARDWAREPATHTRACER_INDEXBUFFER_H
#define HARDWAREPATHTRACER_INDEXBUFFER_H

#include "core/Core.h"


namespace HWPT {
    class IndexBuffer {
    public:
        IndexBuffer(uint IndexCount, void* Data);

        ~IndexBuffer();

        void Bind(VkCommandBuffer CommandBuffer);

        VkBuffer& GetHandle() {
            return m_indexBuffer;
        }

    private:
        VkBuffer m_indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;
    };
}

#endif //HARDWAREPATHTRACER_INDEXBUFFER_H
