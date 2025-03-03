//
// Created by HUSTLX on 2025/2/8.
//

#include "PassBase.h"
#include "core/application/VulkanBackendApp.h"


namespace Shadowy {
    void PassBase::DestroySets() {
        vkDestroyDescriptorSetLayout(GetVKDevice(), m_setLayout, nullptr);
        vkFreeDescriptorSets(GetVKDevice(), VulkanBackendApp::GetApplication()->GetDescriptorPool(),
                             m_sets.size(), m_sets.data());
    }

    void PassBase::DestroyPipeline() {
        m_bindings.clear();
        vkDestroyPipelineLayout(GetVKDevice(), m_pipelineLayout, nullptr);
        vkDestroyPipeline(GetVKDevice(), m_pipeline, nullptr);
    }

    void PassBase::Release() {
        DestroySets();
        DestroyPipeline();
    }

    void PassBase::CreateSets() {
        VkDescriptorSetLayoutCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        CreateInfo.bindingCount = m_bindings.size();
        CreateInfo.pBindings = m_bindings.data();
        CreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
        VK_CHECK(vkCreateDescriptorSetLayout(GetVKDevice(), &CreateInfo, nullptr,
                                             &m_setLayout));

        std::vector<VkDescriptorSetLayout> Layouts(MAX_FRAMES_IN_FLIGHT,
                                                   m_setLayout);
        VkDescriptorSetAllocateInfo AllocateInfo{};
        AllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        AllocateInfo.descriptorPool = VulkanBackendApp::GetApplication()->GetDescriptorPool();
        AllocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        AllocateInfo.pSetLayouts = Layouts.data();

        VkResult Res = vkAllocateDescriptorSets(GetVKDevice(), &AllocateInfo, m_sets.data());
        m_writeSets.reserve(m_bindings.size() * MAX_FRAMES_IN_FLIGHT);
    }

    void PassBase::AddBinding(BindingType Type, uint BindingIndex, uint Count,
                              VkShaderStageFlags ShaderStage) {
        VkDescriptorType DescriptorType;
        switch (Type) {
            case BindingType::TLAS:
                DescriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
                break;
            case BindingType::UniformBuffer:
                DescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                break;
            case BindingType::StorageBuffer:
                DescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                break;
            case BindingType::SampledImage:
                DescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                break;
            case BindingType::StorageImage:
                DescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                break;
        }
        m_bindings.push_back({
                                     BindingIndex, DescriptorType, Count, ShaderStage, nullptr
                             });
    }

    void PassBase::MakeBufferWriteSet(BindingType Type, uint BindingIndex, uint Count,
                                      const std::vector<VkBuffer> &Buffers) {
        std::vector<VkWriteDescriptorSet> Writes(MAX_FRAMES_IN_FLIGHT);
        std::vector<std::vector<VkDescriptorBufferInfo>> BufferInfos(MAX_FRAMES_IN_FLIGHT,
                                                                     std::vector<VkDescriptorBufferInfo>(Count));
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            auto& BufferInfo = BufferInfos[i];
            for (int j = 0; j < Count; j++) {
                BufferInfo[j].buffer = Buffers[i * Count + j];
                BufferInfo[j].offset = 0;
                BufferInfo[j].range = VK_WHOLE_SIZE;
            }

            Writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            Writes[i].dstSet = m_sets[i];
            Writes[i].dstBinding = BindingIndex;
            Writes[i].dstArrayElement = 0;
            switch (Type) {
                case BindingType::UniformBuffer:
                    Writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    break;
                case BindingType::StorageBuffer:
                    Writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    break;
            }
            Writes[i].descriptorCount = Count;
            Writes[i].pBufferInfo = BufferInfo.data();
        }
        vkUpdateDescriptorSets(GetVKDevice(), Writes.size(), Writes.data(), 0, nullptr);
//        m_writeSets.insert(m_writeSets.end(), Writes.begin(), Writes.end());
    }

    void PassBase::MakeImageWriteSet(BindingType Type, uint BindingIndex, uint Count,
                                     const std::vector<VkImageView> &ImageViews) {
        std::vector<VkWriteDescriptorSet> Writes(MAX_FRAMES_IN_FLIGHT);
        std::vector<std::vector<VkDescriptorImageInfo>> ImageInfos(
                MAX_FRAMES_IN_FLIGHT,
                std::vector<VkDescriptorImageInfo>(Count));
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            auto& ImageInfo = ImageInfos[i];
            for (int j = 0; j < Count; j++) {
                switch (Type) {
                    case BindingType::StorageImage:
                        Writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                        ImageInfo[j].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                        break;
                    case BindingType::SampledImage:
                        Writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        ImageInfo[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        break;
                }
                ImageInfo[j].sampler = Sampler::GetDefaultSample()->GetHandle();
                ImageInfo[j].imageView = ImageViews[i * Count + j];
            }

            switch (Type) {
                case BindingType::StorageImage:
                    Writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    break;
                case BindingType::SampledImage:
                    Writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    break;
            }

            Writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            Writes[i].dstSet = m_sets[i];
            Writes[i].dstBinding = BindingIndex;
            Writes[i].dstArrayElement = 0;
            Writes[i].descriptorCount = Count;
            Writes[i].pImageInfo = ImageInfo.data();
        }
        vkUpdateDescriptorSets(GetVKDevice(), Writes.size(), Writes.data(), 0, nullptr);
//        m_writeSets.insert(m_writeSets.end(), Writes.begin(), Writes.end());
    }

    void PassBase::MakeImageWriteSet(BindingType Type, uint BindingIndex, uint Count,
                                     VkImageView ImageView) {
        Check(Count == 1);
        std::vector<VkWriteDescriptorSet> Writes(MAX_FRAMES_IN_FLIGHT);
        std::vector<VkDescriptorImageInfo> ImageInfos(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            auto& ImageInfo = ImageInfos[i];
            switch (Type) {
                case BindingType::StorageImage:
                    Writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                    break;
                case BindingType::SampledImage:
                    Writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    break;
            }
            ImageInfo.sampler = Sampler::GetDefaultSample()->GetHandle();
            ImageInfo.imageView = ImageView;

            switch (Type) {
                case BindingType::StorageImage:
                    Writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    break;
                case BindingType::SampledImage:
                    Writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    break;
            }

            Writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            Writes[i].dstSet = m_sets[i];
            Writes[i].dstBinding = BindingIndex;
            Writes[i].dstArrayElement = 0;
            Writes[i].descriptorCount = Count;
            Writes[i].pImageInfo = &ImageInfo;
        }
        vkUpdateDescriptorSets(GetVKDevice(), Writes.size(), Writes.data(), 0, nullptr);
//        m_writeSets.insert(m_writeSets.end(), Writes.begin(), Writes.end());
    }


    void PassBase::MakeTLASWriteSet(BindingType Type, uint BindingIndex, uint Count,
                                    VkAccelerationStructureKHR TLAS) {
        Check(Count == 1);
        std::vector<VkWriteDescriptorSet> Writes(MAX_FRAMES_IN_FLIGHT);
        std::vector<std::vector<VkWriteDescriptorSetAccelerationStructureKHR>> TLASInfos(
                MAX_FRAMES_IN_FLIGHT,
                std::vector<VkWriteDescriptorSetAccelerationStructureKHR>(
                        Count,
                        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR})
        );

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            auto& TLASInfo = TLASInfos[i];
            TLASInfo[0].accelerationStructureCount = 1;
            TLASInfo[0].pAccelerationStructures = &TLAS;

            Writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            Writes[i].dstSet = m_sets[i];
            Writes[i].dstBinding = BindingIndex;
            Writes[i].dstArrayElement = 0;
            Writes[i].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            Writes[i].descriptorCount = Count;
            Writes[i].pNext = TLASInfo.data();
        }
        vkUpdateDescriptorSets(GetVKDevice(), Writes.size(), Writes.data(), 0, nullptr);
//        m_writeSets.insert(m_writeSets.end(), Writes.begin(), Writes.end());
    }

}  // namespace Shadowy
