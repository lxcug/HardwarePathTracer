//
// Created by HUSTLX on 2025/2/4.
//

#ifndef SHADOWY_RESTIRRESOURCE_H
#define SHADOWY_RESTIRRESOURCE_H

#include "core/Core.h"
#include "core/buffer/ArbitraryBuffer.h"
#include "host_device_shared/ReSTIRCommon.h"


namespace Shadowy {
    class ReSTIRResource {
    public:
        ReSTIRResource(uint Width, uint Height);

        void Init(uint Width, uint Height);

        void Release();

        void OnResize(uint Width, uint Height);

        auto GetReservoirBuffer(int Index) -> std::shared_ptr<ArbitraryBuffer> & {
            return m_reservoirBuffer[Index];
        }

        auto GetLastFrameReservoirBuffer() -> std::shared_ptr<ArbitraryBuffer> & {
            return m_lastFrameReservoirBuffer;
        }

        void CopyToLastFrameReservoirBuffer(VkCommandBuffer CommandBuffer, int Index);

    private:
        std::vector<std::shared_ptr<ArbitraryBuffer>> m_reservoirBuffer;
        std::shared_ptr<ArbitraryBuffer> m_lastFrameReservoirBuffer;
    };

}  // namespace Shadowy

#endif //SHADOWY_RESTIRRESOURCE_H
