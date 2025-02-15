//
// Created by HUSTLX on 2025/1/19.
//

#ifndef SHADOWY_TONEMAPPING_H
#define SHADOWY_TONEMAPPING_H

#include "core/Core.h"
#include "core/texture/Texture2D.h"
#include "host_device_shared/RenderOptions.h"


namespace Shadowy {
    class ToneMappingPass {
    public:
        ToneMappingPass() {
            CreateDescriptorSets();
            CreatePipeline();
        }

        ~ToneMappingPass();

        void DestroyPipeline();

        void CreateDescriptorSets();

        void UpdateDescriptorSets();

        void CreatePipeline();

        void Dispatch(VkCommandBuffer CommandBuffer, uint ImageIndex, uint Width, uint Height);

        auto GetPassRenderOptions() -> ToneMappingOptions& {
            return m_options;
        }

    private:
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_descriptorSets;
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        ToneMappingOptions m_options;
    };

}  // namespace Shadowy

#endif //SHADOWY_TONEMAPPING_H
