//
// Created by HUSTLX on 2025/1/6.
//

#ifndef HARDWAREPATHTRACER_ARBITRARYBUFFER_H
#define HARDWAREPATHTRACER_ARBITRARYBUFFER_H

#include "core/Core.h"


namespace Shadowy {
    class ArbitraryBuffer {
    public:
        ArbitraryBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags MemProps);

        ArbitraryBuffer(VkDeviceSize Size, void* Data, VkBufferUsageFlags Usage, VkMemoryPropertyFlags MemProps);

        ~ArbitraryBuffer();

        auto GetHandle() -> VkBuffer& {
            return m_buffer;
        }

        auto GetMemoryHandle() -> VkDeviceMemory& {
            return m_bufferMemory;
        }

    private:
        VkBuffer m_buffer = VK_NULL_HANDLE;
        VkDeviceMemory m_bufferMemory = VK_NULL_HANDLE;
        VkDeviceAddress m_deviceAddress = 0;
    };
}  // namespace Shadowy

#endif //HARDWAREPATHTRACER_ARBITRARYBUFFER_H
