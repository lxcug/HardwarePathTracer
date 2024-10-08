//
// Created by HUSTLX on 2024/10/8.
//

#ifndef HARDWAREPATHTRACER_UNIFORMBUFFER_H
#define HARDWAREPATHTRACER_UNIFORMBUFFER_H

#include "core/Core.h"


namespace HWPT {
    class UniformBuffer {
    public:
        UniformBuffer(VkDeviceSize Size, void* Data);

        ~UniformBuffer();

    private:
        VkBuffer m_uniformBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_uniformBufferMemory = VK_NULL_HANDLE;
        void* MappedData;
    };
}

#endif //HARDWAREPATHTRACER_UNIFORMBUFFER_H
