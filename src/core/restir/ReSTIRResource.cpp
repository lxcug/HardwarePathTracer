//
// Created by HUSTLX on 2025/2/4.
//

#include "ReSTIRResource.h"
#include "core/application/VulkanBackendApp.h"
#include "core/shader/ShaderBase.h"
#include "core/shader_compiler/HLSLCompiler.h"
#include "core/Utils.h"


namespace Shadowy {
    ReSTIRResource::ReSTIRResource(uint Width, uint Height) : m_width(Width), m_height(Height) {
        Init(Width, Height);
    }

    void ReSTIRResource::OnResize(uint Width, uint Height) {
        m_width = Width;
        m_height = Height;
        for (auto &ReservoirBuffer: m_temporalReservoirBuffer) {
            ReservoirBuffer.reset();
        }
        Init(Width, Height);

        UpdateTemporalReusePassSets();
        UpdateSpatialReusePassSets();
    }

    void ReSTIRResource::Init(uint Width, uint Height) {
        m_initialSampleBuffer.resize(MAX_FRAMES_IN_FLIGHT);
        for (auto &InitialSampleBuffer: m_initialSampleBuffer) {
            InitialSampleBuffer = std::make_shared<ArbitraryBuffer>(
                    sizeof(ReSTIRGISample) * Width * Height,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            );
        }
        m_temporalReservoirBuffer.resize(MAX_FRAMES_IN_FLIGHT);
        for (auto &ReservoirBuffer: m_temporalReservoirBuffer) {
            ReservoirBuffer = std::make_shared<ArbitraryBuffer>(
                    sizeof(ReSTIRGIReservoir) * Width * Height,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            );
        }
        m_spatialReservoirBuffer.resize(MAX_FRAMES_IN_FLIGHT);
        for (auto &ReservoirBuffer: m_spatialReservoirBuffer) {
            ReservoirBuffer = std::make_shared<ArbitraryBuffer>(
                    sizeof(ReSTIRGIReservoir) * Width * Height,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
            );
        }
    }

    void ReSTIRResource::Release() {
        for (auto& InitialSampleBuffer : m_initialSampleBuffer) {
            InitialSampleBuffer.reset();
        }
        for (auto &ReservoirBuffer: m_temporalReservoirBuffer) {
            ReservoirBuffer.reset();
        }
        for (auto &ReservoirBuffer: m_spatialReservoirBuffer) {
            ReservoirBuffer.reset();
        }
        m_initialSampleBuffer.clear();
        m_temporalReservoirBuffer.clear();
        m_spatialReservoirBuffer.clear();

        vkDestroyDescriptorSetLayout(GetVKDevice(), m_temporalReusePassLayout, nullptr);
        vkFreeDescriptorSets(GetVKDevice(), VulkanBackendApp::GetApplication()->GetDescriptorPool(),
                             m_temporalReusePassSets.size(), m_temporalReusePassSets.data());

        vkDestroyDescriptorSetLayout(GetVKDevice(), m_spatialReusePassLayout, nullptr);
        vkFreeDescriptorSets(GetVKDevice(), VulkanBackendApp::GetApplication()->GetDescriptorPool(),
                             m_spatialReusePassSets.size(), m_spatialReusePassSets.data());
    }

    void ReSTIRResource::CreateTemporalReusePassSets() {
        VkDescriptorSetLayoutBinding ViewUniformBuffer{};
        ViewUniformBuffer.binding = 0;
        ViewUniformBuffer.descriptorCount = 1;
        ViewUniformBuffer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ViewUniformBuffer.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding InitialSampleBufferBinding{};
        InitialSampleBufferBinding.binding = 1;
        InitialSampleBufferBinding.descriptorCount = 1;
        InitialSampleBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        InitialSampleBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding TemporalReservoirBufferBinding{};
        TemporalReservoirBufferBinding.binding = 2;
        TemporalReservoirBufferBinding.descriptorCount = 1;
        TemporalReservoirBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        TemporalReservoirBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding LastFrameTemporalReservoirBufferBinding{};
        LastFrameTemporalReservoirBufferBinding.binding = 3;
        LastFrameTemporalReservoirBufferBinding.descriptorCount = 1;
        LastFrameTemporalReservoirBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        LastFrameTemporalReservoirBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding SceneColorBinding{};
        SceneColorBinding.binding = 4;
        SceneColorBinding.descriptorCount = 1;
        SceneColorBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        SceneColorBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        std::array<VkDescriptorSetLayoutBinding, 5> Bindings = {
                ViewUniformBuffer, InitialSampleBufferBinding, TemporalReservoirBufferBinding,
                LastFrameTemporalReservoirBufferBinding, SceneColorBinding
        };
        VkDescriptorSetLayoutCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        CreateInfo.bindingCount = Bindings.size();
        CreateInfo.pBindings = Bindings.data();
        VK_CHECK(vkCreateDescriptorSetLayout(GetVKDevice(), &CreateInfo, nullptr,
                                             &m_temporalReusePassLayout));

        std::vector<VkDescriptorSetLayout> Layouts(MAX_FRAMES_IN_FLIGHT, m_temporalReusePassLayout);

        VkDescriptorSetAllocateInfo AllocateInfo{};
        AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        AllocateInfo.descriptorPool = VulkanBackendApp::GetApplication()->GetDescriptorPool();
        AllocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        AllocateInfo.pSetLayouts = Layouts.data();

        m_temporalReusePassSets.resize(MAX_FRAMES_IN_FLIGHT);
        VK_CHECK(vkAllocateDescriptorSets(GetVKDevice(), &AllocateInfo, m_temporalReusePassSets.data()));
    }

    void ReSTIRResource::UpdateTemporalReusePassSets() {
        std::array<VkWriteDescriptorSet, 5> DescriptorWrites{};
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo ReservoirBufferInfo{};
            ReservoirBufferInfo.buffer = GetViewUniformBuffers()[i]->GetHandle();
            ReservoirBufferInfo.offset = 0;
            ReservoirBufferInfo.range = VK_WHOLE_SIZE;
            DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[0].dstSet = m_temporalReusePassSets[i];
            DescriptorWrites[0].dstBinding = 0;
            DescriptorWrites[0].dstArrayElement = 0;
            DescriptorWrites[0].descriptorCount = 1;
            DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            DescriptorWrites[0].pBufferInfo = &ReservoirBufferInfo;

            VkDescriptorBufferInfo InitialSampleBufferInfo{};
            InitialSampleBufferInfo.buffer = m_initialSampleBuffer[i]->GetHandle();
            InitialSampleBufferInfo.offset = 0;
            InitialSampleBufferInfo.range = m_width * m_height * sizeof(ReSTIRGISample);
            DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[1].dstSet = m_temporalReusePassSets[i];
            DescriptorWrites[1].dstBinding = 1;
            DescriptorWrites[1].dstArrayElement = 0;
            DescriptorWrites[1].descriptorCount = 1;
            DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            DescriptorWrites[1].pBufferInfo = &InitialSampleBufferInfo;

            VkDescriptorBufferInfo TemporalReservoirBufferInfo{};
            TemporalReservoirBufferInfo.buffer = m_temporalReservoirBuffer[i]->GetHandle();
            TemporalReservoirBufferInfo.offset = 0;
            TemporalReservoirBufferInfo.range = m_width * m_height * sizeof(ReSTIRGIReservoir);
            DescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[2].dstSet = m_temporalReusePassSets[i];
            DescriptorWrites[2].dstBinding = 2;
            DescriptorWrites[2].dstArrayElement = 0;
            DescriptorWrites[2].descriptorCount = 1;
            DescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            DescriptorWrites[2].pBufferInfo = &TemporalReservoirBufferInfo;

            int last_frame_index = (i - 1 + MAX_FRAMES_IN_FLIGHT) % MAX_FRAMES_IN_FLIGHT;
            VkDescriptorBufferInfo LastFrameTemporalReservoirBufferInfo{};
            LastFrameTemporalReservoirBufferInfo.buffer = m_temporalReservoirBuffer[last_frame_index]->GetHandle();
            LastFrameTemporalReservoirBufferInfo.offset = 0;
            LastFrameTemporalReservoirBufferInfo.range = m_width * m_height * sizeof(ReSTIRGIReservoir);
            DescriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[3].dstSet = m_temporalReusePassSets[i];
            DescriptorWrites[3].dstBinding = 3;
            DescriptorWrites[3].dstArrayElement = 0;
            DescriptorWrites[3].descriptorCount = 1;
            DescriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            DescriptorWrites[3].pBufferInfo = &LastFrameTemporalReservoirBufferInfo;

            VkDescriptorImageInfo SceneColorInfo{};
            SceneColorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            SceneColorInfo.sampler = Sampler::GetDefaultSample()->GetHandle();
            SceneColorInfo.imageView = GetViewportImages()[i]->CreateSRV();
            DescriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[4].dstSet = m_temporalReusePassSets[i];
            DescriptorWrites[4].dstBinding = 4;
            DescriptorWrites[4].dstArrayElement = 0;
            DescriptorWrites[4].descriptorCount = 1;
            DescriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            DescriptorWrites[4].pImageInfo = &SceneColorInfo;

            vkUpdateDescriptorSets(GetVKDevice(), DescriptorWrites.size(), DescriptorWrites.data(),
                                   0, nullptr);
        }
    }

    void ReSTIRResource::CreateSpatialReusePassSets() {
        VkDescriptorSetLayoutBinding ViewUniformBuffer{};
        ViewUniformBuffer.binding = 0;
        ViewUniformBuffer.descriptorCount = 1;
        ViewUniformBuffer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ViewUniformBuffer.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding TemporalReservoirBufferBinding{};
        TemporalReservoirBufferBinding.binding = 1;
        TemporalReservoirBufferBinding.descriptorCount = 1;
        TemporalReservoirBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        TemporalReservoirBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding SpatialReservoirBufferBinding{};
        SpatialReservoirBufferBinding.binding = 2;
        SpatialReservoirBufferBinding.descriptorCount = 1;
        SpatialReservoirBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        SpatialReservoirBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding SceneColorBinding{};
        SceneColorBinding.binding = 3;
        SceneColorBinding.descriptorCount = 1;
        SceneColorBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        SceneColorBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding TLASBinding{};
        TLASBinding.binding = 4;
        TLASBinding.descriptorCount = 1;
        TLASBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        TLASBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        std::array<VkDescriptorSetLayoutBinding, 5> Bindings = {
                ViewUniformBuffer, TemporalReservoirBufferBinding, SpatialReservoirBufferBinding,
                SceneColorBinding, TLASBinding
        };
        VkDescriptorSetLayoutCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        CreateInfo.bindingCount = Bindings.size();
        CreateInfo.pBindings = Bindings.data();
        VK_CHECK(vkCreateDescriptorSetLayout(GetVKDevice(), &CreateInfo, nullptr,
                                             &m_spatialReusePassLayout));

        std::vector<VkDescriptorSetLayout> Layouts(MAX_FRAMES_IN_FLIGHT, m_spatialReusePassLayout);

        VkDescriptorSetAllocateInfo AllocateInfo{};
        AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        AllocateInfo.descriptorPool = VulkanBackendApp::GetApplication()->GetDescriptorPool();
        AllocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        AllocateInfo.pSetLayouts = Layouts.data();

        m_spatialReusePassSets.resize(MAX_FRAMES_IN_FLIGHT);
        VK_CHECK(vkAllocateDescriptorSets(GetVKDevice(), &AllocateInfo,
                                          m_spatialReusePassSets.data()));
    }

    void ReSTIRResource::UpdateSpatialReusePassSets() {
        std::array<VkWriteDescriptorSet, 5> DescriptorWrites{};
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo ViewUniformBufferInfo{};
            ViewUniformBufferInfo.buffer = GetViewUniformBuffers()[i]->GetHandle();
            ViewUniformBufferInfo.offset = 0;
            ViewUniformBufferInfo.range = VK_WHOLE_SIZE;
            DescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[0].dstSet = m_spatialReusePassSets[i];
            DescriptorWrites[0].dstBinding = 0;
            DescriptorWrites[0].dstArrayElement = 0;
            DescriptorWrites[0].descriptorCount = 1;
            DescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            DescriptorWrites[0].pBufferInfo = &ViewUniformBufferInfo;

            VkDescriptorBufferInfo TemporalReservoirBufferInfo{};
            TemporalReservoirBufferInfo.buffer = m_temporalReservoirBuffer[i]->GetHandle();
            TemporalReservoirBufferInfo.offset = 0;
            TemporalReservoirBufferInfo.range = VK_WHOLE_SIZE;
            DescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[1].dstSet = m_spatialReusePassSets[i];
            DescriptorWrites[1].dstBinding = 1;
            DescriptorWrites[1].dstArrayElement = 0;
            DescriptorWrites[1].descriptorCount = 1;
            DescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            DescriptorWrites[1].pBufferInfo = &TemporalReservoirBufferInfo;

            VkDescriptorBufferInfo SpatialReservoirBufferInfo{};
            SpatialReservoirBufferInfo.buffer = m_spatialReservoirBuffer[i]->GetHandle();
            SpatialReservoirBufferInfo.offset = 0;
            SpatialReservoirBufferInfo.range = VK_WHOLE_SIZE;
            DescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[2].dstSet = m_spatialReusePassSets[i];
            DescriptorWrites[2].dstBinding = 2;
            DescriptorWrites[2].dstArrayElement = 0;
            DescriptorWrites[2].descriptorCount = 1;
            DescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            DescriptorWrites[2].pBufferInfo = &SpatialReservoirBufferInfo;

            VkDescriptorImageInfo SceneColorInfo{};
            SceneColorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            SceneColorInfo.sampler = Sampler::GetDefaultSample()->GetHandle();
            SceneColorInfo.imageView = GetViewportImages()[i]->CreateSRV();
            DescriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[3].dstSet = m_spatialReusePassSets[i];
            DescriptorWrites[3].dstBinding = 3;
            DescriptorWrites[3].dstArrayElement = 0;
            DescriptorWrites[3].descriptorCount = 1;
            DescriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            DescriptorWrites[3].pImageInfo = &SceneColorInfo;

            VkWriteDescriptorSetAccelerationStructureKHR ASInfo{
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR
            };
            ASInfo.accelerationStructureCount = 1;
            auto TLAS = GetTLAS();
            ASInfo.pAccelerationStructures = &TLAS;
            DescriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            DescriptorWrites[4].dstSet = m_spatialReusePassSets[i];
            DescriptorWrites[4].dstBinding = 4;
            DescriptorWrites[4].dstArrayElement = 0;
            DescriptorWrites[4].descriptorCount = 1;
            DescriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            DescriptorWrites[4].pNext = &ASInfo;

            vkUpdateDescriptorSets(GetVKDevice(), DescriptorWrites.size(), DescriptorWrites.data(),
                                   0, nullptr);
        }
    }

    void ReSTIRResource::CreateSpatialReusePassPipeline() {
        VkPushConstantRange PassOptions{};
        PassOptions.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        PassOptions.offset = 0;
        PassOptions.size = sizeof(SpatialReuseOptions);

        VkPipelineLayoutCreateInfo CreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        CreateInfo.setLayoutCount = 1;
        CreateInfo.pSetLayouts = &m_spatialReusePassLayout;
        CreateInfo.pushConstantRangeCount = 1;
        CreateInfo.pPushConstantRanges = &PassOptions;

        VK_CHECK(vkCreatePipelineLayout(GetVKDevice(), &CreateInfo, nullptr,
                                        &m_spatialReusePassPipelineLayout));

        HLSLCompiler::CompileShader("RayTracing/SpatialReuse.hlsl", "SpatialReuse",
                                    ShaderType::Compute,
                                    "RayTracing/SpatialReuse");
        ShaderBase SpatialReuseShader(ShaderType::Compute,
                                      "../../shader/HLSL/RayTracing/SpatialReuse.spv",
                                      "SpatialReuse");

        VkPipelineShaderStageCreateInfo ShaderStage{};
        ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        ShaderStage.module = SpatialReuseShader.GetHandle();
        ShaderStage.pName = SpatialReuseShader.GetEntryName();

        VkComputePipelineCreateInfo PipelineCreateInfo{
                VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        PipelineCreateInfo.stage = ShaderStage;
        PipelineCreateInfo.layout = m_spatialReusePassPipelineLayout;

        VK_CHECK(vkCreateComputePipelines(GetVKDevice(), VK_NULL_HANDLE, 1, &PipelineCreateInfo,
                                          nullptr, &m_spatialReusePassPipeline));
    }

    void ReSTIRResource::CreateTemporalReusePassPipeline() {
        VkPushConstantRange PassOptions{};
        PassOptions.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        PassOptions.offset = 0;
        PassOptions.size = sizeof(TemporalReuseOptions);

        VkPipelineLayoutCreateInfo CreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        CreateInfo.setLayoutCount = 1;
        CreateInfo.pSetLayouts = &m_temporalReusePassLayout;
        CreateInfo.pushConstantRangeCount = 1;
        CreateInfo.pPushConstantRanges = &PassOptions;

        VK_CHECK(vkCreatePipelineLayout(GetVKDevice(), &CreateInfo, nullptr,
                                        &m_temporalReusePassPipelineLayout));

        HLSLCompiler::CompileShader("RayTracing/TemporalReuse.hlsl", "TemporalReuse",
                                    ShaderType::Compute,
                                    "RayTracing/TemporalReuse");
        ShaderBase TemporalReuseShader(ShaderType::Compute,
                                       "../../shader/HLSL/RayTracing/TemporalReuse.spv",
                                       "TemporalReuse");

        VkPipelineShaderStageCreateInfo ShaderStage{};
        ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        ShaderStage.module = TemporalReuseShader.GetHandle();
        ShaderStage.pName = TemporalReuseShader.GetEntryName();

        VkComputePipelineCreateInfo PipelineCreateInfo{
                VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        PipelineCreateInfo.stage = ShaderStage;
        PipelineCreateInfo.layout = m_temporalReusePassPipelineLayout;

        VK_CHECK(vkCreateComputePipelines(GetVKDevice(), VK_NULL_HANDLE, 1, &PipelineCreateInfo,
                                          nullptr, &m_temporalReusePassPipeline));
    }

    void ReSTIRResource::DispatchTemporalReusePass(VkCommandBuffer CommandBuffer,
                                                   Texture2D* SceneColor,
                                                   uint ImageIndex,
                                                   uint Width, uint Height) {
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          m_temporalReusePassPipeline);
        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                m_temporalReusePassPipelineLayout,
                                0, 1, &m_temporalReusePassSets[ImageIndex], 0, nullptr);
        vkCmdPushConstants(CommandBuffer, m_temporalReusePassPipelineLayout,
                           VK_SHADER_STAGE_COMPUTE_BIT,
                           0, sizeof(TemporalReuseOptions), &m_temporalReuseOptions);
        // TODO: now threadgroup size is fixed in shader
        glm::uvec3 ThreadGroupCount = Utils::GetThreadGroupCount({Width, Height, 1}, {16, 16, 1});
        vkCmdDispatch(CommandBuffer, ThreadGroupCount.x, ThreadGroupCount.y, ThreadGroupCount.z);
    }

    void ReSTIRResource::DispatchSpatialReusePass(VkCommandBuffer CommandBuffer,
                                                  Texture2D* SceneColor,
                                                  uint ImageIndex,
                                                  uint Width, uint Height) {
        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          m_spatialReusePassPipeline);
        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                m_spatialReusePassPipelineLayout,
                                0, 1, &m_spatialReusePassSets[ImageIndex], 0, nullptr);
        vkCmdPushConstants(CommandBuffer, m_spatialReusePassPipelineLayout,
                           VK_SHADER_STAGE_COMPUTE_BIT,
                           0, sizeof(SpatialReuseOptions), &m_spatialReuseOptions);
        // TODO: now threadgroup size is fixed in shader
        glm::uvec3 ThreadGroupCount = Utils::GetThreadGroupCount({Width, Height, 1}, {16, 16, 1});
        vkCmdDispatch(CommandBuffer, ThreadGroupCount.x, ThreadGroupCount.y, ThreadGroupCount.z);
    }

}  // namespace Shadowy
