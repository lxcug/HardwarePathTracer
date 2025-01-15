//
// Created by HUSTLX on 2025/1/14.
//

#include "GBuffer.h"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "core/application/VulkanBackendApp.h"


namespace HWPT
{
    void GBuffer::Init()
    {
        m_albedo.resize(m_frameCount);
        m_normal.resize(m_frameCount);
        m_pos.resize(m_frameCount);
        m_depth.resize(m_frameCount);
        m_albedoDescriptorSets.resize(m_frameCount);
        m_normalDescriptorSets.resize(m_frameCount);
        m_posDescriptorSets.resize(m_frameCount);
        m_depthDescriptorSets.resize(m_frameCount);

        auto CommandBuffer = RHI::BeginIntermediateCommandBuffer();
        RHI::TextureTransitionInput SrcInput{}, DstInput{};
        SrcInput.Layout = VK_IMAGE_LAYOUT_UNDEFINED;
        SrcInput.AccessMask = 0;
        SrcInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
        DstInput.Layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        DstInput.AccessMask = 0;
        DstInput.PipelineStage = VK_PIPELINE_STAGE_NONE;
        auto Sampler = Sampler::GetDefaultSample().GetHandle();
        for (int i = 0; i < m_frameCount; i++)
        {
            m_albedo[i] = new Texture2D(m_size.x, m_size.y,
                                        TextureFormat::RGBA, TextureUsage::UAV);
            m_normal[i] = new Texture2D(m_size.x, m_size.y,
                                        TextureFormat::RGBA,
                                        TextureUsage::UAV);
            m_pos[i] = new Texture2D(m_size.x, m_size.y,
                                     TextureFormat::RGBA, TextureUsage::UAV);
            m_depth[i] = new Texture2D(m_size.x, m_size.y,
                                       TextureFormat::RGBA, TextureUsage::UAV);

            RHI::TransitionTextureLayout(CommandBuffer, m_albedo[i]->GetHandle(), 1, SrcInput,
                                         DstInput);
            RHI::TransitionTextureLayout(CommandBuffer, m_normal[i]->GetHandle(), 1,
                                         SrcInput, DstInput);
            RHI::TransitionTextureLayout(CommandBuffer, m_pos[i]->GetHandle(), 1, SrcInput,
                                         DstInput);
            RHI::TransitionTextureLayout(CommandBuffer, m_depth[i]->GetHandle(), 1, SrcInput,
                                         DstInput);

            m_albedoDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(
                Sampler,
                m_albedo[i]->CreateSRV(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            m_normalDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(
                Sampler,
                m_normal[i]->CreateSRV(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            m_posDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(
                Sampler,
                m_pos[i]->CreateSRV(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            m_depthDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(
                Sampler,
                m_depth[i]->CreateSRV(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        RHI::SubmitIntermediateCommandBuffer(CommandBuffer);
    }

    void GBuffer::Release()
    {
        vkDeviceWaitIdle(GetVKDevice());

        for (int i = 0; i < m_frameCount; i++)
        {
            ImGui_ImplVulkan_RemoveTexture(m_albedoDescriptorSets[i]);
            ImGui_ImplVulkan_RemoveTexture(m_normalDescriptorSets[i]);
            ImGui_ImplVulkan_RemoveTexture(m_posDescriptorSets[i]);
            ImGui_ImplVulkan_RemoveTexture(m_depthDescriptorSets[i]);
            delete m_albedo[i];
            delete m_normal[i];
            delete m_pos[i];
            delete m_depth[i];
        }
    }
} // namespace HWPT
