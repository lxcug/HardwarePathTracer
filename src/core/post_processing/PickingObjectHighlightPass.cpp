//
// Created by HUSTLX on 2025/3/8.
//

#include "PickingObjectHighlightPass.h"
#include "core/application/VulkanRayTracingApp.h"


namespace Shadowy {

    void PickingObjectHighlightPass::CreateSets() {
        AddBinding(BindingType::StorageImage, 0, 1, VK_SHADER_STAGE_COMPUTE_BIT);
        AddBinding(BindingType::StorageBuffer, 1, 1, VK_SHADER_STAGE_COMPUTE_BIT);
        AddBinding(BindingType::StorageImage, 2, 1, VK_SHADER_STAGE_COMPUTE_BIT);
        PassBase::CreateSets();
    }

    void PickingObjectHighlightPass::UpdateSets() {
        MakeImageWriteSet(BindingType::StorageImage, 0, 1,
                          {
                              GetGlobalTexture(TextureType::InstanceID, 0)->CreateSRV(),
                              GetGlobalTexture(TextureType::InstanceID, 1)->CreateSRV()
                          });
        MakeBufferWriteSet(BindingType::StorageBuffer, 1, 1,
                           reinterpret_cast<VulkanRayTracingApp*>(VulkanBackendApp::GetApplication())->GetPickingInstanceIDBuffer()->GetHandle());
        MakeImageWriteSet(BindingType::StorageImage, 2, 1,
                          {
                              GetGlobalTexture(TextureType::SceneColor, 0)->CreateSRV(),
                              GetGlobalTexture(TextureType::SceneColor, 1)->CreateSRV(),
                          });
        PassBase::UpdateSets();
    }

    void PickingObjectHighlightPass::CreatePipeline() {
        VkPipelineLayoutCreateInfo CreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        CreateInfo.setLayoutCount = 1;
        CreateInfo.pSetLayouts = &m_setLayout;
        CreateInfo.pushConstantRangeCount = 0;
        VK_CHECK(vkCreatePipelineLayout(GetVKDevice(), &CreateInfo, nullptr, &m_pipelineLayout));

        HLSLCompiler::CompileShader("PostProcess/PickingHighlight.hlsl", "PickingHighlight",
                                    ShaderType::Compute,
                                    "PostProcess/PickingHighlight");
        ShaderBase PickingHighlightShader(ShaderType::Compute,
                                          "../../shader/HLSL/PostProcess/PickingHighlight.spv",
                                          "PickingHighlight");

        VkPipelineShaderStageCreateInfo ShaderStage{};
        ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        ShaderStage.module = PickingHighlightShader.GetHandle();
        ShaderStage.pName = PickingHighlightShader.GetEntryName();

        VkComputePipelineCreateInfo PipelineCreateInfo{
                VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        PipelineCreateInfo.stage = ShaderStage;
        PipelineCreateInfo.layout = m_pipelineLayout;

        VK_CHECK(vkCreateComputePipelines(GetVKDevice(), VK_NULL_HANDLE, 1, &PipelineCreateInfo,
                                          nullptr, &m_pipeline));
    }

    void PickingObjectHighlightPass::Dispatch(VkCommandBuffer CommandBuffer, uint ImageIndex) {
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout,
                                0, 1, &m_sets[ImageIndex], 0, nullptr);
        // TODO: now threadgroup size is fixed in shader
        glm::uvec3 ThreadGroupCount = Utils::GetThreadGroupCount({m_width, m_height, 1}, {16, 16, 1});
        vkCmdDispatch(CommandBuffer, ThreadGroupCount.x, ThreadGroupCount.y, ThreadGroupCount.z);
    }
}