//
// Created by HUSTLX on 2024/10/8.
//

#ifndef HARDWAREPATHTRACER_STORAGEBUFFER_H
#define HARDWAREPATHTRACER_STORAGEBUFFER_H

#include "core/Core.h"


namespace HWPT {
    class StorageBuffer {
    public:
        StorageBuffer(VkDeviceSize Size, void* Data);

        ~StorageBuffer();

        auto GetHandle() -> VkBuffer& {
            return m_storageBuffer;
        }

    private:
        VkBuffer m_storageBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_storageBufferMemory = VK_NULL_HANDLE;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_STORAGEBUFFER_H
