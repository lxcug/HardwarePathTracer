//
// Created by HUSTLX on 2025/3/2.
//

#include "BloomPass.h"


namespace Shadowy {
    void BloomPass::CreateSets()
    {
        AddBinding(BindingType::StorageImage, 0, 1, VK_SHADER_STAGE_COMPUTE_BIT);
        AddBinding(BindingType::StorageImage, 1, 1, VK_SHADER_STAGE_COMPUTE_BIT);

        PassBase::CreateSets();
    }

    void BloomPass::UpdateSets()
    {
        MakeImageWriteSet(BindingType::StorageImage, 0, 1,
                  {
                          GetGlobalTexture(TextureType::SceneColor, 0)->CreateSRV(),
                          GetGlobalTexture(TextureType::SceneColor, 1)->CreateSRV()
                  });
        MakeImageWriteSet(BindingType::StorageImage, 1, 1, {
            m_tempBrightness[0]->CreateSRV(),
            m_tempBrightness[1]->CreateSRV()
        });

        PassBase::UpdateSets();
    }

    void BloomPass::CreatePipeline()
    {
        // Brightness Extract Pass
        VkPipelineLayoutCreateInfo CreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        CreateInfo.setLayoutCount = 1;
        CreateInfo.pSetLayouts = &m_setLayout;
        VK_CHECK(vkCreatePipelineLayout(GetVKDevice(), &CreateInfo, nullptr, &m_pipelineLayout));
        
        HLSLCompiler::CompileShader("PostProcess/Bloom.hlsl", "BrightnessExtract",
                            ShaderType::Compute,
                            "PostProcess/BrightnessExtract");
        ShaderBase BrightnessExtractShader(ShaderType::Compute,
                                      "../../shader/HLSL/PostProcess/BrightnessExtract.spv",
                                      "BrightnessExtract");
        VkPipelineShaderStageCreateInfo ShaderStage{};
        ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        ShaderStage.module = BrightnessExtractShader.GetHandle();
        ShaderStage.pName = BrightnessExtractShader.GetEntryName();
        VkComputePipelineCreateInfo PipelineCreateInfo{
            VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        PipelineCreateInfo.stage = ShaderStage;
        PipelineCreateInfo.layout = m_pipelineLayout;
        VK_CHECK(vkCreateComputePipelines(GetVKDevice(), VK_NULL_HANDLE, 1, &PipelineCreateInfo,
                                          nullptr, &m_brightnessExtractPipeline));

        HLSLCompiler::CompileShader("PostProcess/Bloom.hlsl", "HorizontalBlur",
                            ShaderType::Compute,
                            "PostProcess/HorizontalBlur");
        ShaderBase HorizontalBlurPassShader(ShaderType::Compute,
                                      "../../shader/HLSL/PostProcess/HorizontalBlur.spv",
                                      "HorizontalBlur");
        ShaderStage.module = HorizontalBlurPassShader.GetHandle();
        ShaderStage.pName = HorizontalBlurPassShader.GetEntryName();
        PipelineCreateInfo.stage = ShaderStage;
        VK_CHECK(vkCreateComputePipelines(GetVKDevice(), VK_NULL_HANDLE, 1, &PipelineCreateInfo,
                                          nullptr, &m_horizontalBlurPipeline));

        HLSLCompiler::CompileShader("PostProcess/Bloom.hlsl", " VerticalBlur",
                    ShaderType::Compute,
                    "PostProcess/VerticalBlur");
        ShaderBase VerticalBlurBlurPassShader(ShaderType::Compute,
                                      "../../shader/HLSL/PostProcess/VerticalBlur.spv",
                                      "VerticalBlur");
        ShaderStage.module = VerticalBlurBlurPassShader.GetHandle();
        ShaderStage.pName = VerticalBlurBlurPassShader.GetEntryName();
        PipelineCreateInfo.stage = ShaderStage;
        VK_CHECK(vkCreateComputePipelines(GetVKDevice(), VK_NULL_HANDLE, 1, &PipelineCreateInfo,
                                          nullptr, &m_verticalBlurPipeline));

        HLSLCompiler::CompileShader("PostProcess/Bloom.hlsl", "Composite",
                                    ShaderType::Compute,
                                    "PostProcess/Composite");
        ShaderBase CompositePassShader(ShaderType::Compute,
                                      "../../shader/HLSL/PostProcess/Composite.spv",
                                      "Composite");
        ShaderStage.module = CompositePassShader.GetHandle();
        ShaderStage.pName = CompositePassShader.GetEntryName();
        PipelineCreateInfo.stage = ShaderStage;
        VK_CHECK(vkCreateComputePipelines(GetVKDevice(), VK_NULL_HANDLE, 1, &PipelineCreateInfo,
                                          nullptr, &m_pipeline));
    }

    void BloomPass::Dispatch(VkCommandBuffer CommandBuffer, uint ImageIndex)
    {
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_brightnessExtractPipeline);
        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout,
            0, 1, &m_sets[ImageIndex], 0, nullptr);
        // TODO: now threadgroup size is fixed in shader
        glm::uvec3 ThreadGroupCount = Utils::GetThreadGroupCount({m_width, m_height, 1}, {16, 16, 1});
        vkCmdDispatch(CommandBuffer, ThreadGroupCount.x, ThreadGroupCount.y, ThreadGroupCount.z);

        VkImageMemoryBarrier Barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        Barrier.image = m_tempBrightness[ImageIndex]->GetHandle();
        Barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        Barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        Barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        Barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        Barrier.subresourceRange.baseMipLevel = 0;
        Barrier.subresourceRange.levelCount = 1;
        Barrier.subresourceRange.baseArrayLayer = 0;
        Barrier.subresourceRange.layerCount = 1;
        Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkCmdPipelineBarrier(CommandBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 0, nullptr,
            0, nullptr, 1, &Barrier);
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_horizontalBlurPipeline);
        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout,
            0, 1, &m_sets[ImageIndex], 0, nullptr);
        vkCmdDispatch(CommandBuffer, ThreadGroupCount.x, ThreadGroupCount.y, ThreadGroupCount.z);

        vkCmdPipelineBarrier(CommandBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 0, nullptr,
            0, nullptr, 1, &Barrier);
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_verticalBlurPipeline);
        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout,
            0, 1, &m_sets[ImageIndex], 0, nullptr);
        vkCmdDispatch(CommandBuffer, ThreadGroupCount.x, ThreadGroupCount.y, ThreadGroupCount.z);

        vkCmdPipelineBarrier(CommandBuffer,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    0, 0, nullptr,
    0, nullptr, 1, &Barrier);
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_horizontalBlurPipeline);
        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout,
            0, 1, &m_sets[ImageIndex], 0, nullptr);
        vkCmdDispatch(CommandBuffer, ThreadGroupCount.x, ThreadGroupCount.y, ThreadGroupCount.z);

        vkCmdPipelineBarrier(CommandBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 0, nullptr,
            0, nullptr, 1, &Barrier);
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_verticalBlurPipeline);
        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout,
            0, 1, &m_sets[ImageIndex], 0, nullptr);
        vkCmdDispatch(CommandBuffer, ThreadGroupCount.x, ThreadGroupCount.y, ThreadGroupCount.z);

        vkCmdPipelineBarrier(CommandBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 0, nullptr,
            0, nullptr, 1, &Barrier);
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);
        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout,
            0, 1, &m_sets[ImageIndex], 0, nullptr);
        vkCmdDispatch(CommandBuffer, ThreadGroupCount.x, ThreadGroupCount.y, ThreadGroupCount.z);
    }
}  // namespace Shadowy