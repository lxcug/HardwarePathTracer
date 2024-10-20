//
// Created by HUSTLX on 2024/10/18.
//

#include "RenderPassCommon.h"
#include "ShaderParameters.h"


namespace HWPT {
    auto GetVKPrimitiveType(PrimitiveType Type) -> VkPrimitiveTopology {
        switch (Type) {
            case PrimitiveType::Triangle:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case PrimitiveType::TriangleStrip:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case PrimitiveType::TriangleFan:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
            case PrimitiveType::Line:
                return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case PrimitiveType::LineStrip:
                return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            case PrimitiveType::Point:
                return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            default:
                throw std::runtime_error("Unsupported PrimitiveType");
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    }

    RenderPassBase::~RenderPassBase() {
        vkDestroyPipelineLayout(GetVKDevice(), m_pipelineLayout, nullptr);
        vkDestroyPipeline(GetVKDevice(), m_pipeline, nullptr);
        delete m_parameters;
    }

    void RenderPassBase::SetShaderParameters(ShaderParameters *PassParameters) {
        if (PassParameters->m_renderPass != this) {
            throw std::runtime_error("ShaderParameters::RenderPass is nullptr or mismatch");
        }
        m_parameters = PassParameters;
    }

    void RenderPassBase::CreatePipelineLayout() {
        VkPipelineLayoutCreateInfo PipelineLayoutInfo{};
        PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        PipelineLayoutInfo.setLayoutCount = 1;
        PipelineLayoutInfo.pSetLayouts = &m_parameters->GetDescriptorSetLayout();
        VkShaderStageFlags ShaderStage = VK_SHADER_STAGE_ALL_GRAPHICS;
        if (m_passFlag == PassFlag::Compute) {
            ShaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
        }
        // TODO: PushConstant Support
        VkPushConstantRange ConstantRange{};
        ConstantRange.stageFlags = ShaderStage;
        ConstantRange.offset = 0;
        ConstantRange.size = 0;
        PipelineLayoutInfo.pushConstantRangeCount = 0;

        VK_CHECK(vkCreatePipelineLayout(GetVKDevice(), &PipelineLayoutInfo, nullptr,
                                        &m_pipelineLayout));
    }
}  // namespace HWPT
