//
// Created by HUSTLX on 2025/1/14.
//

#ifndef GBUFFER_H
#define GBUFFER_H

#include "core/Core.h"
#include "core/texture/Texture2D.h"
#include "core/RHI.h"
#include "core/PassBase.h"
#include "core/buffer/UniformBuffer.h"
#include "core/buffer/ArbitraryBuffer.h"
#include "core/application/GlobalTexture.h"


namespace Shadowy
{
    class GBufferPass : public PassBase {
    public:
        void Init(uint Width, uint Height) override;

        void CreateSets() override;

        void UpdateSets() override;

        void CreatePipeline() override;

        void Release() override {
            PassBase::Release();
        }

        // TODO: Profile
        void Dispatch(VkCommandBuffer CommandBuffer, uint ImageIndex) override;

        void OnResize(uint Width, uint Height) override;

    protected:
        enum StageIndices : uint8_t {
            RayGen,
            Miss,
            ClosestHit,
            AnyHit,
            Intersection,
            StageIndicesMax
        };
        VkStridedDeviceAddressRegionKHR m_rayGenRegion{};
        VkStridedDeviceAddressRegionKHR m_missRegion{};
        VkStridedDeviceAddressRegionKHR m_hitRegion{};
        VkStridedDeviceAddressRegionKHR m_callRegion{};
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_RTProps{
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR
        };
        std::shared_ptr<ArbitraryBuffer> m_RTSBTBuffer;
    };
} // namespace Shadowy

#endif //GBUFFER_H
