//
// Created by HUSTLX on 2025/2/8.
//

#ifndef SHADOWY_PATHTRACINGPASS_H
#define SHADOWY_PATHTRACINGPASS_H

#include "core/Core.h"
#include "core/PassBase.h"
#include "core/buffer/UniformBuffer.h"
#include "core/buffer/ArbitraryBuffer.h"
#include "core/application/GlobalTexture.h"


namespace Shadowy {
    class PathTracingPass : public PassBase {
    public:
        void Init();



    private:
        VkDescriptorSetLayout m_RTDescriptorSetLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_RTDescriptorSets;
        VkPipelineLayout m_RTPipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_RTPipeline = VK_NULL_HANDLE;

        enum StageIndices : uint8_t {
            RayGen,
            Miss,
            ClosestHit,
            AnyHit,
            Intersection,
            StageIndicesMax
        };
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_RTShaderGroups;
        ArbitraryBuffer *m_RTSBTBuffer = nullptr;
        VkStridedDeviceAddressRegionKHR m_rayGenRegion{};
        VkStridedDeviceAddressRegionKHR m_missRegion{};
        VkStridedDeviceAddressRegionKHR m_hitRegion{};
        VkStridedDeviceAddressRegionKHR m_callRegion{};
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_RTProps{
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR
        };
    };

}  // namespace Shadowy

#endif //SHADOWY_PATHTRACINGPASS_H
