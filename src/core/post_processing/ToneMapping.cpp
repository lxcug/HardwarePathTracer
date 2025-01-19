//
// Created by HUSTLX on 2025/1/19.
//

#include "ToneMapping.h"
#include "core/application/VulkanBackendApp.h"
#include "core/shader/ShaderBase.h"
#include "core/shader_compiler/HLSLCompiler.h"
#include "core/Utils.h"


namespace Shadowy {

    ToneMappingPass::~ToneMappingPass() {
        vkDestroyDescriptorSetLayout(GetVKDevice(), m_descriptorSetLayout, nullptr);
        vkFreeDescriptorSets(GetVKDevice(), VulkanBackendApp::GetApplication()->GetDescriptorPool(),
                             MAX_FRAMES_IN_FLIGHT, m_descriptorSets.data());
        vkDestroyPipelineLayout(GetVKDevice(), m_pipelineLayout, nullptr);
        vkDestroyPipeline(GetVKDevice(), m_pipeline, nullptr);
    }

    void ToneMappingPass::CreateDescriptorSets() {
        VkDescriptorSetLayoutBinding InImageBinding{};
        InImageBinding.binding = 0;
        InImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        InImageBinding.descriptorCount = 1;
        InImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo CreateInfo{
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        CreateInfo.bindingCount = 1;
        CreateInfo.pBindings = &InImageBinding;
        VK_CHECK(vkCreateDescriptorSetLayout(GetVKDevice(), &CreateInfo, nullptr,
                                             &m_descriptorSetLayout));

        std::vector<VkDescriptorSetLayout> Layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
        VkDescriptorSetAllocateInfo AllocateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        AllocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        AllocateInfo.pSetLayouts = Layouts.data();
        AllocateInfo.descriptorPool = VulkanBackendApp::GetApplication()->GetDescriptorPool();

        m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        VK_CHECK(vkAllocateDescriptorSets(GetVKDevice(), &AllocateInfo, m_descriptorSets.data()));
    }

    void ToneMappingPass::UpdateDescriptorSets(std::vector<Texture2D *> SceneColors) {
        std::array<VkWriteDescriptorSet, 1> DescriptorWrites{};
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorImageInfo SceneColorBinding{};
            SceneColorBinding.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            SceneColorBinding.imageView = SceneColors[i]->CreateSRV();
            DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[0].dstSet = m_descriptorSets[i];
            DescriptorWrites[0].dstBinding = 0;
            DescriptorWrites[0].dstArrayElement = 0;
            DescriptorWrites[0].descriptorCount = 1;
            DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            DescriptorWrites[0].pImageInfo = &SceneColorBinding;

            vkUpdateDescriptorSets(GetVKDevice(), DescriptorWrites.size(), DescriptorWrites.data(),
                                   0, nullptr);
        }
    }

    void ToneMappingPass::CreatePipeline() {
        VkPushConstantRange PushConstantRange{};
        PushConstantRange.offset = 0;
        PushConstantRange.size = sizeof(ToneMappingOptions);
        PushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkPipelineLayoutCreateInfo CreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        CreateInfo.setLayoutCount = 1;
        CreateInfo.pSetLayouts = &m_descriptorSetLayout;
        CreateInfo.pushConstantRangeCount = 1;
        CreateInfo.pPushConstantRanges = &PushConstantRange;

        VK_CHECK(vkCreatePipelineLayout(GetVKDevice(), &CreateInfo, nullptr, &m_pipelineLayout));

        HLSLCompiler::CompileShader("PostProcess/ToneMapping.hlsl", "ToneMapping",
                                    ShaderType::Compute,
                                    "PostProcess/ToneMapping");
        ShaderBase ToneMappingShader(ShaderType::Compute,
                                     "../../shader/HLSL/PostProcess/ToneMapping.spv",
                                     "ToneMapping");

        VkPipelineShaderStageCreateInfo ShaderStage{};
        ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        ShaderStage.module = ToneMappingShader.GetHandle();
        ShaderStage.pName = ToneMappingShader.GetEntryName();

        VkComputePipelineCreateInfo PipelineCreateInfo{
                VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        PipelineCreateInfo.stage = ShaderStage;
        PipelineCreateInfo.layout = m_pipelineLayout;

        VK_CHECK(vkCreateComputePipelines(GetVKDevice(), VK_NULL_HANDLE, 1, &PipelineCreateInfo,
                                          nullptr, &m_pipeline));
    }

    void ToneMappingPass::Dispatch(VkCommandBuffer CommandBuffer, uint ImageIndex, uint Width,
                                   uint Height) {
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout,
                                0, 1, &m_descriptorSets[ImageIndex], 0, nullptr);
        vkCmdPushConstants(CommandBuffer, m_pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                           0, sizeof(ToneMappingOptions), &m_options);
        // TODO: now threadgroup size is fixed in shader
        glm::uvec3 ThreadGroupCount = Utils::GetThreadGroupCount({Width, Height, 1}, {16, 16, 1});
        vkCmdDispatch(CommandBuffer, ThreadGroupCount.x, ThreadGroupCount.y, ThreadGroupCount.z);
    }

}  // namespace Shadowy
