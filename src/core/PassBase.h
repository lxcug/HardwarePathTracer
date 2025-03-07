//
// Created by HUSTLX on 2025/2/8.
//

#ifndef SHADOWY_PASSBASE_H
#define SHADOWY_PASSBASE_H

#include "core/Core.h"
#include "core/texture/Sampler.h"
#include "core/Utils.h"
#include "core/shader_compiler/HLSLCompiler.h"
#include "core/application/GlobalTexture.h"
#include "host_device_shared/RenderOptions.h"


namespace Shadowy {
    enum class BindingType {
        TLAS = 0x0,
        UniformBuffer,
        StorageBuffer,
        SampledImage,
        StorageImage
    };

    class PassBase {
    public:
        virtual void Init(uint Width, uint Height) {
            m_width = Width;
            m_height = Height;
            m_sets.resize(MAX_FRAMES_IN_FLIGHT);
        }

        virtual void OnResize(uint Width, uint Height) {
            m_width = Width;
            m_height = Height;
        }

        virtual void CreateSets();

        virtual void UpdateSets() {
            // TODO: Not update sets here cause stack memory release for Image/Buffer/AS Infos
//            vkUpdateDescriptorSets(GetVKDevice(), m_writeSets.size(), m_writeSets.data(), 0, nullptr);
//            m_writeSets.clear();
//            m_writeSets.reserve(m_bindings.size() * MAX_FRAMES_IN_FLIGHT);
        }

        virtual void CreatePipeline() = 0;

        virtual void Dispatch(VkCommandBuffer CommandBuffer, uint ImageIndex) = 0;

        virtual void Release();

        void AddBinding(BindingType Type, uint BindingIndex, uint Count,
                        VkShaderStageFlags ShaderStage);

        void MakeBufferWriteSet(BindingType Type,
                                uint BindingIndex,
                                uint Count,
                                VkBuffer Buffer);

        void MakeBufferWriteSet(BindingType Type,
                                uint BindingIndex,
                                uint Count,
                                const std::vector<VkBuffer>& Buffers);

        void MakeImageWriteSet(BindingType Type,
                               uint BindingIndex,
                               uint Count,
                               const std::vector<VkImageView>& ImageView);

        void MakeImageWriteSet(BindingType Type,
                               uint BindingIndex,
                               uint Count,
                               VkImageView ImageView);

        void MakeTLASWriteSet(BindingType Type,
                              uint BindingIndex,
                              uint Count,
                              VkAccelerationStructureKHR TLAS);

        virtual void DestroySets();

        virtual void DestroyPipeline();

        virtual void OnRecompile() {
            DestroyPipeline();
            CreatePipeline();
        }

    protected:
        VkDescriptorSetLayout m_setLayout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_sets;

        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_pipeline = VK_NULL_HANDLE;

        uint m_width = 0, m_height = 0;

        std::vector<VkDescriptorSetLayoutBinding> m_bindings;
        std::vector<VkWriteDescriptorSet> m_writeSets;
    };

}  // namespace Shadowy

#endif //SHADOWY_PASSBASE_H
