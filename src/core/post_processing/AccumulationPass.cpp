//
// Created by HUSTLX on 2025/2/12.
//

#include "AccumulationPass.h"
#include "core/application/VulkanBackendApp.h"


namespace Shadowy {

    void AccumulationPass::CreateSets() {
        AddBinding(BindingType::UniformBuffer, 0, 1, VK_SHADER_STAGE_COMPUTE_BIT);
        AddBinding(BindingType::StorageImage, 1, 1, VK_SHADER_STAGE_COMPUTE_BIT);
        AddBinding(BindingType::StorageImage, 2, 1, VK_SHADER_STAGE_COMPUTE_BIT);

        PassBase::CreateSets();
    }

    void AccumulationPass::UpdateSets() {
        MakeBufferWriteSet(BindingType::UniformBuffer, 0, 1,
                           {
                                   GetViewUniformBuffers()[0]->GetHandle(),
                                   GetViewUniformBuffers()[1]->GetHandle()
                           });
        MakeImageWriteSet(BindingType::StorageImage, 1, 1,
                          {
                                  GetGlobalTexture(TextureType::SceneColor, 0)->CreateSRV(),
                                  GetGlobalTexture(TextureType::SceneColor, 1)->CreateSRV()
                          });
        MakeImageWriteSet(BindingType::StorageImage, 2, 1,
                          GetLastFrameSceneColor()->CreateSRV());
        PassBase::UpdateSets();
    }

    void AccumulationPass::Dispatch(VkCommandBuffer CommandBuffer, uint ImageIndex) {
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout,
                                0, 1, &m_sets[ImageIndex], 0, nullptr);
        vkCmdPushConstants(CommandBuffer, m_pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                           0, sizeof(AccumulationOptions), &m_options);
        // TODO: now threadgroup size is fixed in shader
        glm::uvec3 ThreadGroupCount = Utils::GetThreadGroupCount({m_width, m_height, 1}, {16, 16, 1});
        vkCmdDispatch(CommandBuffer, ThreadGroupCount.x, ThreadGroupCount.y, ThreadGroupCount.z);
    }

    void AccumulationPass::CreatePipeline() {
        VkPushConstantRange PushConstant{};
        PushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        PushConstant.offset = 0;
        PushConstant.size = sizeof(AccumulationOptions);
        VkPipelineLayoutCreateInfo CreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        CreateInfo.setLayoutCount = 1;
        CreateInfo.pSetLayouts = &m_setLayout;
        CreateInfo.pushConstantRangeCount = 1;
        CreateInfo.pPushConstantRanges = &PushConstant;
        VK_CHECK(vkCreatePipelineLayout(GetVKDevice(), &CreateInfo, nullptr, &m_pipelineLayout));

        HLSLCompiler::CompileShader("PostProcess/Accumulation.hlsl", "Accumulation",
                                    ShaderType::Compute,
                                    "PostProcess/Accumulation");
        ShaderBase AccumulationShader(ShaderType::Compute,
                                      "../../shader/HLSL/PostProcess/Accumulation.spv",
                                      "Accumulation");

        VkPipelineShaderStageCreateInfo ShaderStage{};
        ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        ShaderStage.module = AccumulationShader.GetHandle();
        ShaderStage.pName = AccumulationShader.GetEntryName();

        VkComputePipelineCreateInfo PipelineCreateInfo{
                VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        PipelineCreateInfo.stage = ShaderStage;
        PipelineCreateInfo.layout = m_pipelineLayout;

        VK_CHECK(vkCreateComputePipelines(GetVKDevice(), VK_NULL_HANDLE, 1, &PipelineCreateInfo,
                                          nullptr, &m_pipeline));
    }


}  // namespace Shadowy

