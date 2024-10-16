//
// Created by HUSTLX on 2024/10/8.
//

#ifndef HARDWAREPATHTRACER_UNIFORMBUFFER_H
#define HARDWAREPATHTRACER_UNIFORMBUFFER_H

#include "core/Core.h"


namespace HWPT {
    class UniformBuffer {
    public:
        UniformBuffer(VkDeviceSize Size, const void *Data);

        ~UniformBuffer();

        void Update(void* Data);

        auto GetHandle() -> VkBuffer& {
            return m_uniformBuffer;
        }

    private:
        VkBuffer m_uniformBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_uniformBufferMemory = VK_NULL_HANDLE;
        void* m_mappedData = nullptr;
        VkDeviceSize m_size = 0;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_UNIFORMBUFFER_H
