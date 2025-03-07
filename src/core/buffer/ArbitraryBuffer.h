//
// Created by HUSTLX on 2025/1/6.
//

#ifndef HARDWAREPATHTRACER_ARBITRARYBUFFER_H
#define HARDWAREPATHTRACER_ARBITRARYBUFFER_H

#include "core/Core.h"


namespace Shadowy {
    class ArbitraryBuffer {
    public:
        ArbitraryBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags MemProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        ArbitraryBuffer(VkDeviceSize Size, void* Data, VkBufferUsageFlags Usage, VkMemoryPropertyFlags MemProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        ~ArbitraryBuffer();

        void Update(VkDeviceSize Size, void* Data);

        void Update(void* Data);

        auto GetHandle() -> VkBuffer& {
            return m_buffer;
        }

        auto GetMemoryHandle() -> VkDeviceMemory& {
            return m_bufferMemory;
        }

        void ReadBuffer(void* &OutData);

    private:
        VkBuffer m_buffer = VK_NULL_HANDLE;
        VkDeviceMemory m_bufferMemory = VK_NULL_HANDLE;
        VkDeviceSize m_size;
    };
}  // namespace Shadowy

#endif //HARDWAREPATHTRACER_ARBITRARYBUFFER_H
