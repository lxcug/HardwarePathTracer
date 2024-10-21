//
// Created by HUSTLX on 2024/10/18.
//

#include "ComputePass.h"
#include "core/renderGraph/ShaderParameters.h"
#include <utility>
#include "core/application/VulkanBackendApp.h"


namespace HWPT {

    ComputePass::ComputePass(std::string PassName, PassFlag Flag, const std::string &ComputeSPVPath,
                             const char *ComputeEntry)
            : RenderPassBase(std::move(PassName), Flag) {
        m_computeShader = new ShaderBase(ShaderType::Compute, ComputeSPVPath, ComputeEntry);
    }

    ComputePass::~ComputePass() {
        delete m_computeShader;
    }

    void ComputePass::OnRenderPassSetupFinish() {
        Check(m_parameters != nullptr);
        m_parameters->OnShaderParameterSetFinish();  // Create DescriptorSetLayout
        CreatePipelineLayout();
        CreateRenderPipeline();
    }

    void ComputePass::BindRenderPass(VkCommandBuffer CommandBuffer) {
        m_parameters->OnRenderPassBegin();
        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0,
                                1, &m_parameters->GetDescriptorSets(), 0, nullptr);
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
    }

    void ComputePass::CreateRenderPipeline() {
        VkPipelineShaderStageCreateInfo ComputeShaderStageInfo{};
        ComputeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ComputeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        ComputeShaderStageInfo.module = m_computeShader->GetHandle();
        ComputeShaderStageInfo.pName = m_computeShader->GetEntryName();

        VkComputePipelineCreateInfo PipelineInfo{};
        PipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        PipelineInfo.stage = ComputeShaderStageInfo;
        PipelineInfo.layout = m_pipelineLayout;

        VK_CHECK(vkCreateComputePipelines(GetVKDevice(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr,
                                          &m_pipeline));
    }

    void
    ComputePass::Execute(uint NumThreadGroupX, uint NumThreadGroupY, uint NumThreadGroupZ) {
        vkCmdDispatch(m_commandBuffer, NumThreadGroupX, NumThreadGroupY, NumThreadGroupZ);
    }
}  // namespace HWPT
