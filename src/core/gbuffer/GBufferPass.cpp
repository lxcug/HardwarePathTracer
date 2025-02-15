//
// Created by HUSTLX on 2025/1/14.
//

#include "GBufferPass.h"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "core/application/VulkanBackendApp.h"
#include "core/shader/ShaderBase.h"
#include "core/shader_compiler/HLSLCompiler.h"
#include "core/scene/Scene.h"
#include "core/Utils.h"


namespace Shadowy
{
    void GBufferPass::Init(uint Width, uint Height) {
        PassBase::Init(Width, Height);

        CreateGlobalTexture(TextureType::Albedo_Metallic, m_width, m_height,
                            VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL);
        CreateGlobalTexture(TextureType::Normal_Roughness, m_width, m_height,
                            VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL);
        CreateGlobalTexture(TextureType::WorldPosition, m_width, m_height,
                            VK_FORMAT_R8G8B8A8_SNORM, VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL);
        CreateGlobalTexture(TextureType::SceneDepth, m_width, m_height,
                            VK_FORMAT_R16_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL);
        CreateGlobalTexture(TextureType::MotionVector, m_width, m_height,
                            VK_FORMAT_R16G16_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL);
        CreateGlobalTexture(TextureType::Penumbra, m_width, m_height,
                            VK_FORMAT_R16_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL);
        CreateGlobalTexture(TextureType::Translucency, m_width, m_height,
                            VK_FORMAT_R8G8B8A8_SNORM, VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL);
    }

    void GBufferPass::CreateSets() {
        AddBinding(BindingType::UniformBuffer, 0, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        AddBinding(BindingType::TLAS, 1, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        AddBinding(BindingType::StorageImage, 2, 3, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        AddBinding(BindingType::StorageImage, 3, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        AddBinding(BindingType::StorageImage, 4, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        AddBinding(BindingType::StorageImage, 5, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        AddBinding(BindingType::StorageImage, 6, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        AddBinding(BindingType::StorageImage, 7, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

        PassBase::CreateSets();
    }

    void GBufferPass::UpdateSets() {
        std::vector<VkWriteDescriptorSet> Writes;
        Writes.reserve(MAX_FRAMES_IN_FLIGHT * m_bindings.size());

        MakeBufferWriteSet(BindingType::UniformBuffer, 0, 1,
                           {
                                   GetViewUniformBuffers()[0]->GetHandle(),
                                   GetViewUniformBuffers()[1]->GetHandle()
                           });
        MakeTLASWriteSet(BindingType::TLAS, 1, 1, GetTLAS());
        MakeImageWriteSet(BindingType::StorageImage, 2, 3,
                          {
                                  GetGlobalTexture(TextureType::Albedo_Metallic, 0)->CreateSRV(),
                                  GetGlobalTexture(TextureType::Normal_Roughness, 0)->CreateSRV(),
                                  GetGlobalTexture(TextureType::WorldPosition, 0)->CreateSRV(),
                                  GetGlobalTexture(TextureType::Albedo_Metallic, 1)->CreateSRV(),
                                  GetGlobalTexture(TextureType::Normal_Roughness, 1)->CreateSRV(),
                                  GetGlobalTexture(TextureType::WorldPosition, 1)->CreateSRV(),
                          });
        MakeImageWriteSet(BindingType::StorageImage, 3, 1,
                          {
                                  GetGlobalTexture(TextureType::SceneDepth, 0)->CreateSRV(),
                                  GetGlobalTexture(TextureType::SceneDepth, 1)->CreateSRV()
                          });
        MakeImageWriteSet(BindingType::StorageImage, 4, 1,
                          {
                                  GetGlobalTexture(TextureType::MotionVector, 0)->CreateSRV(),
                                  GetGlobalTexture(TextureType::MotionVector, 1)->CreateSRV()
                          });
        MakeImageWriteSet(BindingType::StorageImage, 5, 1,
                          {
                                  GetGlobalTexture(TextureType::Penumbra, 0)->CreateSRV(),
                                  GetGlobalTexture(TextureType::Penumbra, 1)->CreateSRV()
                          });
        MakeImageWriteSet(BindingType::StorageImage, 6, 1,
                          {
                                  GetGlobalTexture(TextureType::Translucency, 0)->CreateSRV(),
                                  GetGlobalTexture(TextureType::Translucency, 1)->CreateSRV()
                          });
        MakeImageWriteSet(BindingType::StorageImage, 7, 1,
                          {
                                  GetGlobalTexture(TextureType::SceneColor, 0)->CreateSRV(),
                                  GetGlobalTexture(TextureType::SceneColor, 1)->CreateSRV()
                          });

        PassBase::UpdateSets();
    }

    void GBufferPass::CreatePipeline() {
        VkPipelineLayoutCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        std::array<VkDescriptorSetLayout, 2> Layouts = {
                m_setLayout,
                GetScene()->GetSceneDescriptorSetLayout()
        };
        CreateInfo.setLayoutCount = Layouts.size();
        CreateInfo.pSetLayouts = Layouts.data();

        VK_CHECK(vkCreatePipelineLayout(GetVKDevice(), &CreateInfo, nullptr, &m_pipelineLayout));

        HLSLCompiler::CompileShader("RayTracing/GBuffer/GBuffer_raygen.hlsl", "main", ShaderType::RayGen,
                                    "RayTracing/GBuffer/GBuffer_raygen");
        HLSLCompiler::CompileShader("RayTracing/GBuffer/GBuffer_miss.hlsl", "main", ShaderType::Miss,
                                    "RayTracing/GBuffer/GBuffer_miss");
        HLSLCompiler::CompileShader("RayTracing/GBuffer/GBuffer_closesthit.hlsl", "main",
                                    ShaderType::ClosestHit,
                                    "RayTracing/GBuffer/GBuffer_closesthit");

        // Create Shader Modules
        ShaderBase RayGenShader(ShaderType::RayGen, "../../shader/HLSL/RayTracing/GBuffer/GBuffer_raygen.spv",
                                "main");
        ShaderBase MissShader(ShaderType::Miss, "../../shader/HLSL/RayTracing/GBuffer/GBuffer_miss.spv",
                              "main");
        ShaderBase ClosestHitShader(ShaderType::ClosestHit,
                                    "../../shader/HLSL/RayTracing/GBuffer/GBuffer_closesthit.spv", "main");

        std::array<VkPipelineShaderStageCreateInfo, 3> ShaderStages{};
        VkPipelineShaderStageCreateInfo ShaderStage{};
        ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        ShaderStage.module = RayGenShader.GetHandle();
        ShaderStage.pName = RayGenShader.GetEntryName();
        ShaderStages[RayGen] = ShaderStage;
        ShaderStage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
        ShaderStage.module = MissShader.GetHandle();
        ShaderStage.pName = MissShader.GetEntryName();
        ShaderStages[Miss] = ShaderStage;
        ShaderStage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        ShaderStage.module = ClosestHitShader.GetHandle();
        ShaderStage.pName = ClosestHitShader.GetEntryName();
        ShaderStages[ClosestHit] = ShaderStage;

        // Create Shader Group
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> RTShaderGroups;
        VkRayTracingShaderGroupCreateInfoKHR ShaderGroupCreateInfo{};
        ShaderGroupCreateInfo.sType =
                VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        ShaderGroupCreateInfo.generalShader = VK_SHADER_UNUSED_KHR;
        ShaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
        ShaderGroupCreateInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
        ShaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

        ShaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        ShaderGroupCreateInfo.generalShader = RayGen;
        RTShaderGroups.push_back(ShaderGroupCreateInfo);

        ShaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        ShaderGroupCreateInfo.generalShader = Miss;
        RTShaderGroups.push_back(ShaderGroupCreateInfo);

        ShaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        ShaderGroupCreateInfo.generalShader = ClosestHit;
        RTShaderGroups.push_back(ShaderGroupCreateInfo);

        VkRayTracingPipelineCreateInfoKHR PipelineCreateInfo{};
        PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        PipelineCreateInfo.stageCount = ShaderStages.size();
        PipelineCreateInfo.pStages = ShaderStages.data();
        PipelineCreateInfo.groupCount = RTShaderGroups.size();
        PipelineCreateInfo.pGroups = RTShaderGroups.data();
        PipelineCreateInfo.layout = m_pipelineLayout;
        PipelineCreateInfo.maxPipelineRayRecursionDepth = 10;

        auto CreateRTPipelineFunc = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
                vkGetDeviceProcAddr(
                        GetVKDevice(), "vkCreateRayTracingPipelinesKHR"));
        VK_CHECK(CreateRTPipelineFunc(GetVKDevice(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1,
                                      &PipelineCreateInfo, nullptr, &m_pipeline));

        VkPhysicalDeviceProperties2 Props2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        Props2.pNext = &m_RTProps;
        vkGetPhysicalDeviceProperties2(GetVKPhysicalDevice(), &Props2);
        uint RayGenCount = 1, MissCount = 1, HitCount = 1;
        uint HandleCount = RayGenCount + MissCount + HitCount;

        uint HandleSizeAligned = Utils::Align(m_RTProps.shaderGroupHandleSize,
                                              m_RTProps.shaderGroupBaseAlignment);

        m_rayGenRegion.stride = Utils::Align(HandleSizeAligned,
                                             m_RTProps.shaderGroupBaseAlignment);
        m_rayGenRegion.size = m_rayGenRegion.stride;

        m_missRegion.stride = HandleSizeAligned;
        m_missRegion.size = Utils::Align(MissCount * HandleSizeAligned,
                                         m_RTProps.shaderGroupBaseAlignment);

        m_hitRegion.stride = HandleSizeAligned;
        m_hitRegion.size = Utils::Align(HitCount * HandleSizeAligned,
                                        m_RTProps.shaderGroupBaseAlignment);

        uint DataSize = HandleCount * m_RTProps.shaderGroupHandleSize;
        std::vector<uint8_t> Handles(DataSize);
        auto Func = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
                vkGetDeviceProcAddr(
                        GetVKDevice(), "vkGetRayTracingShaderGroupHandlesKHR"));
        VK_CHECK(Func(GetVKDevice(), m_pipeline, 0, HandleCount, DataSize, Handles.data()));

        VkDeviceSize SBTSize = m_rayGenRegion.size + m_missRegion.size + m_hitRegion.size;
        m_RTSBTBuffer = std::make_shared<ArbitraryBuffer>(SBTSize,
                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                            VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // Find the SBT addresses of each group
        VkDeviceAddress SBTAddress = RHI::GetBufferDeviceAddress(m_RTSBTBuffer->GetHandle());
        m_rayGenRegion.deviceAddress = SBTAddress;
        m_missRegion.deviceAddress = SBTAddress + m_rayGenRegion.size;
        m_hitRegion.deviceAddress = SBTAddress + m_rayGenRegion.size + m_missRegion.size;

        uint HandleIndex = 0;
        void *MappedData;
        vkMapMemory(GetVKDevice(), m_RTSBTBuffer->GetMemoryHandle(), 0, SBTSize, 0, &MappedData);
        auto pData = reinterpret_cast<uint8_t *>(MappedData);
        for (int i = 0; i < RayGenCount; i++) {
            memcpy(reinterpret_cast<void *>(pData),
                   Handles.data() + HandleIndex++ * m_RTProps.shaderGroupHandleSize,
                   m_RTProps.shaderGroupHandleSize);
            pData += m_rayGenRegion.stride;
        }
        for (int i = 0; i < MissCount; i++) {
            memcpy(reinterpret_cast<void *>(pData),
                   Handles.data() + HandleIndex++ * m_RTProps.shaderGroupHandleSize,
                   m_RTProps.shaderGroupHandleSize);
            pData += m_missRegion.stride;
        }
        for (int i = 0; i < HitCount; i++) {
            memcpy(reinterpret_cast<void *>(pData),
                   Handles.data() + HandleIndex++ * m_RTProps.shaderGroupHandleSize,
                   m_RTProps.shaderGroupHandleSize);
            pData += m_hitRegion.stride;
        }
        vkUnmapMemory(GetVKDevice(), m_RTSBTBuffer->GetMemoryHandle());
    }

    void GBufferPass::OnResize(uint Width, uint Height) {
        PassBase::OnResize(Width, Height);

        GetGlobalTextureHandle()->ReleaseGlobalTexture(
                {
                        TextureType::Albedo_Metallic,
                        TextureType::Normal_Roughness,
                        TextureType::WorldPosition,
                        TextureType::SceneDepth,
                        TextureType::MotionVector,
                        TextureType::Penumbra,
                        TextureType::Translucency
                });
        Init(Width, Height);
        UpdateSets();
    }

    void GBufferPass::Dispatch(VkCommandBuffer CommandBuffer, uint ImageIndex) {
        vkCmdBindPipeline(CommandBuffer,
                          VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                          m_pipeline);
        std::array<VkDescriptorSet, 2> BindSets{
            m_sets[ImageIndex],
            GetScene()->GetModelDescDescriptorSet(ImageIndex)
        };
        vkCmdBindDescriptorSets(CommandBuffer,
                                VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                                m_pipelineLayout,
                                0, BindSets.size(),
                                BindSets.data(),
                                0, nullptr);
        auto RayTraceFunc = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(
                GetVKDevice(), "vkCmdTraceRaysKHR"));
        RayTraceFunc(CommandBuffer, &m_rayGenRegion, &m_missRegion, &m_hitRegion,
                     &m_callRegion, m_width, m_height, 1);
    }
} // namespace Shadowy
