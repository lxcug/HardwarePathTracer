//
// Created by HUSTLX on 2024/10/19.
//

#include "ShaderParameters.h"
#include "core/buffer/StorageBuffer.h"
#include "core/buffer/UniformBuffer.h"
#include "core/texture/Texture2D.h"
#include "RasterPass.h"
#include "core/application/VulkanBackendApp.h"


namespace HWPT {
    ShaderParameters::ShaderParameters(RenderPassBase *RenderPass)
            : m_renderPass(RenderPass) {}

    ShaderParameters::~ShaderParameters() {
        vkDestroyDescriptorSetLayout(GetVKDevice(), m_descriptorSetLayout, nullptr);
    }

    void
    ShaderParameters::AddParameter(const std::string &ParameterName, ShaderParameterType Type) {
        if (m_parameterMetaData.count(ParameterName) != 0) {
            throw std::runtime_error("Duplicate Name in ShaderParameters");
        }

        ShaderParameterMetaData MetaData{};
        MetaData.ParameterName = ParameterName;
        MetaData.ParameterType = Type;
        m_parameterMetaData[ParameterName] = MetaData;
    }

    void ShaderParameters::CreateDescriptorSetLayout() {
        std::vector<VkDescriptorSetLayoutBinding> Bindings;
        uint BindingIndex = 0;

        VkShaderStageFlags ShaderStage = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        if (m_renderPass->GetPassFlag() == PassFlag::Raster) {
            auto Raster = dynamic_cast<RasterPass *>(m_renderPass);
            if (Raster->HasGeometryShader()) {
                ShaderStage |= VK_SHADER_STAGE_GEOMETRY_BIT;
            }
        } else if (m_renderPass->GetPassFlag() == PassFlag::Compute) {
            ShaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
        }

        for (auto &It: m_parameterMetaData) {
            auto &MetaData = It.second;

            switch (MetaData.ParameterType) {
                case ShaderParameterType::UniformBuffer: {
                    VkDescriptorSetLayoutBinding UniformBufferLayoutBinding{};
                    UniformBufferLayoutBinding.binding = BindingIndex++;
                    UniformBufferLayoutBinding.descriptorCount = 1;
                    UniformBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    UniformBufferLayoutBinding.stageFlags = ShaderStage;
                    Bindings.push_back(UniformBufferLayoutBinding);
                    break;
                }
                case ShaderParameterType::Texture2D:
                    [[fallthrough]];
                case ShaderParameterType::RWTexture2D: {
                    VkDescriptorSetLayoutBinding TextureLayoutBinding{};
                    TextureLayoutBinding.binding = BindingIndex++;
                    TextureLayoutBinding.descriptorCount = 1;
                    TextureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    TextureLayoutBinding.stageFlags = ShaderStage;
                    Bindings.push_back(TextureLayoutBinding);
                    break;
                }
                case ShaderParameterType::StorageBuffer:
                    [[fallthrough]];
                case ShaderParameterType::RWStorageBuffer: {
                    VkDescriptorSetLayoutBinding StorageBufferLayoutBinding{};
                    StorageBufferLayoutBinding.binding = BindingIndex++;
                    StorageBufferLayoutBinding.descriptorCount = 1;
                    StorageBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    StorageBufferLayoutBinding.stageFlags = ShaderStage;
                    Bindings.push_back(StorageBufferLayoutBinding);
                    break;
                }
                default:;
            }
        }

        VkDescriptorSetLayoutCreateInfo LayoutInfo{};
        LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        LayoutInfo.bindingCount = Bindings.size();
        LayoutInfo.pBindings = Bindings.data();

        VK_CHECK(vkCreateDescriptorSetLayout(GetVKDevice(), &LayoutInfo, nullptr,
                                             &m_descriptorSetLayout));
    }

    void ShaderParameters::CreateDescriptorSets() {
        VkDescriptorSetAllocateInfo AllocateInfo{};
        AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        AllocateInfo.descriptorPool = VulkanBackendApp::GetApplication()->GetGlobalDescriptorPool();
        AllocateInfo.descriptorSetCount = 1;
        AllocateInfo.pSetLayouts = &m_descriptorSetLayout;
        if (m_descriptorSet != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(GetVKDevice(), VulkanBackendApp::GetApplication()->GetGlobalDescriptorPool(), 1, &m_descriptorSet);
        }
        VK_CHECK(vkAllocateDescriptorSets(GetVKDevice(), &AllocateInfo, &m_descriptorSet));

        std::vector<VkWriteDescriptorSet> DescriptorWrites;
        uint BindingIndex = 0;
        for (auto &It: m_parameterMetaData) {
            auto &MetaData = It.second;

            switch (MetaData.ParameterType) {
                case ShaderParameterType::UniformBuffer: {
                    Check(MetaData.Parameter != nullptr);
                    VkDescriptorBufferInfo BufferInfo{};
                    BufferInfo.buffer = static_cast<UniformBuffer *>(MetaData.Parameter)->GetHandle();
                    BufferInfo.offset = 0;
                    BufferInfo.range = VK_WHOLE_SIZE;
                    MetaData.ParameterDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    MetaData.ParameterDescriptorSet.dstSet = m_descriptorSet;
                    MetaData.ParameterDescriptorSet.dstBinding = BindingIndex++;
                    MetaData.ParameterDescriptorSet.dstArrayElement = 0;
                    MetaData.ParameterDescriptorSet.descriptorCount = 1;
                    MetaData.ParameterDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    MetaData.ParameterDescriptorSet.pBufferInfo = &BufferInfo;
                    DescriptorWrites.push_back(MetaData.ParameterDescriptorSet);
                    break;
                }
                case ShaderParameterType::Texture2D: {
                    Check(MetaData.Parameter != nullptr);
                    VkDescriptorImageInfo ImageInfo{};
                    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
                    ImageInfo.imageView = static_cast<Texture2D *>(MetaData.Parameter)->CreateSRV();
                    ImageInfo.sampler = VulkanBackendApp::GetApplication()->GetGlobalSampler()->GetHandle();  // TODO
                    MetaData.ParameterDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    MetaData.ParameterDescriptorSet.dstSet = m_descriptorSet;
                    MetaData.ParameterDescriptorSet.dstBinding = BindingIndex++;
                    MetaData.ParameterDescriptorSet.dstArrayElement = 0;
                    MetaData.ParameterDescriptorSet.descriptorCount = 1;
                    MetaData.ParameterDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    MetaData.ParameterDescriptorSet.pImageInfo = &ImageInfo;
                    DescriptorWrites.push_back(MetaData.ParameterDescriptorSet);
                    break;
                }
                case ShaderParameterType::RWTexture2D: {
                    Check(MetaData.Parameter != nullptr);
                    VkDescriptorImageInfo ImageInfo{};
                    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                    ImageInfo.imageView = static_cast<Texture2D *>(MetaData.Parameter)->CreateSRV();
                    ImageInfo.sampler = VulkanBackendApp::GetApplication()->GetGlobalSampler()->GetHandle();  // TODO
                    MetaData.ParameterDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    MetaData.ParameterDescriptorSet.dstSet = m_descriptorSet;
                    MetaData.ParameterDescriptorSet.dstBinding = BindingIndex++;
                    MetaData.ParameterDescriptorSet.dstArrayElement = 0;
                    MetaData.ParameterDescriptorSet.descriptorCount = 1;
                    MetaData.ParameterDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    MetaData.ParameterDescriptorSet.pImageInfo = &ImageInfo;
                    DescriptorWrites.push_back(MetaData.ParameterDescriptorSet);
                    break;
                }
                case ShaderParameterType::StorageBuffer:
                    [[fallthrough]];
                case ShaderParameterType::RWStorageBuffer : {
                    Check(MetaData.Parameter != nullptr);
                    VkDescriptorBufferInfo BufferInfo{};
                    BufferInfo.buffer = static_cast<UniformBuffer *>(MetaData.Parameter)->GetHandle();
                    BufferInfo.offset = 0;
                    BufferInfo.range = VK_WHOLE_SIZE;
                    MetaData.ParameterDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    MetaData.ParameterDescriptorSet.dstSet = m_descriptorSet;
                    MetaData.ParameterDescriptorSet.dstBinding = BindingIndex++;
                    MetaData.ParameterDescriptorSet.dstArrayElement = 0;
                    MetaData.ParameterDescriptorSet.descriptorCount = 1;
                    MetaData.ParameterDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    MetaData.ParameterDescriptorSet.pBufferInfo = &BufferInfo;
                    DescriptorWrites.push_back(MetaData.ParameterDescriptorSet);
                    break;
                }
                default:;
            }
        }

        if (!DescriptorWrites.empty()) {
            vkUpdateDescriptorSets(GetVKDevice(), DescriptorWrites.size(), DescriptorWrites.data(),
                                   0, nullptr);
        }
    }

//    void ShaderParameters::PushConstants(VkCommandBuffer CommandBuffer) {
//        for (auto &It: m_parameterMetaData) {
//            auto &MetaData = It.second;
//
//            switch (MetaData.ParameterType) {
//                case ShaderParameterType::Float:
//                case ShaderParameterType::Int:
//                case ShaderParameterType::UInt:
//                    SetConstant<4>(CommandBuffer, MetaData);
//                    break;
//
//                case ShaderParameterType::Float2:
//                case ShaderParameterType::Int2:
//                case ShaderParameterType::UInt2:
//                    SetConstant<8>(CommandBuffer, MetaData);
//                    break;
//
//                case ShaderParameterType::Float3:
//                case ShaderParameterType::Int3:
//                case ShaderParameterType::UInt3:
//                    SetConstant<12>(CommandBuffer, MetaData);
//                    break;
//
//                case ShaderParameterType::Float4:
//                case ShaderParameterType::Int4:
//                case ShaderParameterType::UInt4:
//                    SetConstant<16>(CommandBuffer, MetaData);
//
//                default:;
//            }
//        }
//    }

    void ShaderParameters::OnShaderParameterSetFinish() {
        if (!m_parameterSetFinish) {
            CreateDescriptorSetLayout();
            m_parameterSetFinish = true;
        }
    }

    void ShaderParameters::AddParameters(
            const std::initializer_list<std::pair<std::string, ShaderParameterType>> &InitList) {
        for (auto& Elem : InitList) {
            AddParameter(Elem.first, Elem.second);
        }
    }

    void ShaderParameters::OnRenderPassBegin() {
        CreateDescriptorSets();
    }

//    template<uint ConstantSize>
//    void ShaderParameters::SetConstant(VkCommandBuffer CommandBuffer,
//                                       ShaderParameterMetaData &MetaData) {
//        // TODO Get ShaderStage From RenderPass
//        vkCmdPushConstants(CommandBuffer, nullptr,
//                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
//                           0, ConstantSize, &MetaData.Parameter);
//    }
}  // namespace HWPT
