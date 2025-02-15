//
// Created by HUSTLX on 2025/2/4.
//

#ifndef SHADOWY_RESTIRRESOURCE_H
#define SHADOWY_RESTIRRESOURCE_H

#include "core/Core.h"
#include "core/buffer/ArbitraryBuffer.h"
#include "host_device_shared/ReSTIRCommon.h"
#include "core/texture/Texture2D.h"
#include "core/buffer/UniformBuffer.h"
#include "host_device_shared/RenderOptions.h"


namespace Shadowy {
    class ReSTIRResource {
    public:
        ReSTIRResource(uint Width, uint Height);

        ~ReSTIRResource() {
            Release();
            DestroyPipeline();
        }

        void Init(uint Width, uint Height);

        void Release();

        void OnResize(uint Width, uint Height);

        void DestroyPipeline() {
            vkDestroyPipelineLayout(GetVKDevice(), m_temporalReusePassPipelineLayout, nullptr);
            vkDestroyPipeline(GetVKDevice(), m_temporalReusePassPipeline, nullptr);
            vkDestroyPipelineLayout(GetVKDevice(), m_spatialReusePassPipelineLayout, nullptr);
            vkDestroyPipeline(GetVKDevice(), m_spatialReusePassPipeline, nullptr);
        }

        void CreateTemporalReusePassSets();

        void CreateSpatialReusePassSets();

        void UpdateTemporalReusePassSets();

        void UpdateSpatialReusePassSets();

        void CreateTemporalReusePassPipeline();

        void DispatchTemporalReusePass(VkCommandBuffer CommandBuffer, Texture2D* SceneColor, uint ImageIndex, uint Width, uint Height);

        void CreateSpatialReusePassPipeline();

        void DispatchSpatialReusePass(VkCommandBuffer CommandBuffer, Texture2D* SceneColor, uint ImageIndex, uint Width, uint Height);

        auto GetSpatialReuseOptions() -> SpatialReuseOptions& {
            return m_spatialReuseOptions;
        }

        auto GetTemporalReuseOptions() -> TemporalReuseOptions& {
            return m_temporalReuseOptions;
        }

        auto GetInitialSampleBuffer(uint ImageIndex) -> std::shared_ptr<ArbitraryBuffer>& {
            return m_initialSampleBuffer[ImageIndex];
        }

    private:
        std::vector<std::shared_ptr<ArbitraryBuffer>> m_initialSampleBuffer;
        std::vector<std::shared_ptr<ArbitraryBuffer>> m_temporalReservoirBuffer;
        std::vector<std::shared_ptr<ArbitraryBuffer>> m_spatialReservoirBuffer;

        VkDescriptorSetLayout m_temporalReusePassLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_temporalReusePassSets;
        VkPipelineLayout m_temporalReusePassPipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_temporalReusePassPipeline = VK_NULL_HANDLE;

        VkDescriptorSetLayout m_spatialReusePassLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_spatialReusePassSets;
        VkPipelineLayout m_spatialReusePassPipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_spatialReusePassPipeline = VK_NULL_HANDLE;

        SpatialReuseOptions m_spatialReuseOptions;
        TemporalReuseOptions m_temporalReuseOptions;

        int m_width, m_height;
    };

}  // namespace Shadowy

#endif //SHADOWY_RESTIRRESOURCE_H
