//
// Created by HUSTLX on 2025/2/4.
//

#include "ReSTIRResource.h"


namespace Shadowy {
    ReSTIRResource::ReSTIRResource(uint Width, uint Height) {
        Init(Width, Height);
    }

    void ReSTIRResource::OnResize(uint Width, uint Height) {
        Release();
        Init(Width, Height);
    }

    void ReSTIRResource::Init(uint Width, uint Height) {
        m_reservoirBuffer.resize(MAX_FRAMES_IN_FLIGHT);
        for (auto & ReservoirBuffer : m_reservoirBuffer) {
            ReservoirBuffer = std::make_shared<ArbitraryBuffer>(
                    sizeof(Reservoir) * Width * Height,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
        }
        m_lastFrameReservoirBuffer = std::make_shared<ArbitraryBuffer>(
                sizeof(Reservoir) * Width * Height,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
    }

    void ReSTIRResource::Release() {
        for (auto & ReservoirBuffer : m_reservoirBuffer) {
            ReservoirBuffer.reset();
        }
        m_lastFrameReservoirBuffer.reset();
    }

    void ReSTIRResource::CopyToLastFrameReservoirBuffer(VkCommandBuffer CommandBuffer, int Index) {
        VkBufferCopy CopyInfo{};
        CopyInfo.srcOffset = 0;
        CopyInfo.dstOffset = 0;
        CopyInfo.size = VK_WHOLE_SIZE;
        vkCmdCopyBuffer(CommandBuffer,
                        m_reservoirBuffer[Index]->GetHandle(),
                        m_lastFrameReservoirBuffer->GetHandle(),
                        1, &CopyInfo);
    }
}  // namespace Shadowy
