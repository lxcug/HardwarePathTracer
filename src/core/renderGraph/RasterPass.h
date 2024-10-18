//
// Created by HUSTLX on 2024/10/18.
//

#ifndef HARDWAREPATHTRACER_RASTERPASS_H
#define HARDWAREPATHTRACER_RASTERPASS_H

#include "RenderPassCommon.h"
#include <optional>
#include "core/shader/ShaderBase.h"
#include "core/buffer/VertexBufferLayout.h"
#include "ShaderParameters.h"


namespace HWPT {
    struct RasterPassShaders {
        ShaderBase* VertexShader = nullptr;
        ShaderBase* GeometryShader = nullptr;
        ShaderBase* FragmentShader = nullptr;

        ~RasterPassShaders() {
            delete VertexShader;
            delete GeometryShader;
            delete FragmentShader;
        }
    };

    class RasterPass : public RenderPassBase {
    public:
        RasterPass(std::string PassName, PassFlag Flag,
                   const std::string& VertexSPVPath,
                   const char* VertexEntry,
                   const std::string& FragSPVPath,
                   const char* FragEntry,
                   PrimitiveType InPrimitiveType = PrimitiveType::Triangle);

        RasterPass(std::string PassName, PassFlag Flag,
                   const std::string& VertexSPVPath,
                   const char* VertexEntry,
                   const std::string& GeometrySPVPath,
                   const char* GeometryEntry,
                   const std::string& FragSPVPath,
                   const char* FragEntry,
                   PrimitiveType InPrimitiveType = PrimitiveType::Triangle);

        ~RasterPass() override;

        void SetVertexBufferLayout(const std::initializer_list<VertexAttribute>& VertexAttributes);

        void CreateRenderPipeline();

        void BindRenderPipeline(VkCommandBuffer CommandBuffer) const;

//        void BindDescriptorSets(VkCommandBuffer CommandBuffer, VkDescriptorSet& DescriptorSet);

        auto GetDescriptorSetLayout() -> VkDescriptorSetLayout {
            return m_descriptorSetLayouts[0];
        }

        auto GetPipelineLayout() -> VkPipelineLayout {
            return m_pipelineLayout;
        }

        auto GetPipeline() -> VkPipeline {
            return m_pipeline;
        }

        auto GetRenderPass() -> VkRenderPass {
            return m_renderPass;
        };

    private:
        void Init();

        void CreateDefaultVertexBufferLayout();

        // TODO: Shader Parameters
        void CreatePipelineDescriptorSetLayouts();

        void CreatePipelineLayout();

        void CreateRenderPass();

    private:
        RasterPassShaders m_shaders;
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VertexBufferLayout* m_vertexBufferLayout = nullptr;
        std::vector<VkDescriptorSetLayout> m_descriptorSetLayouts;
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        ShaderParameters* m_parameters = nullptr;  // TODO
        VkRenderPass m_renderPass = VK_NULL_HANDLE;  // TODO: sub-pass support
        PrimitiveType m_primitiveType = PrimitiveType::None;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_RASTERPASS_H
