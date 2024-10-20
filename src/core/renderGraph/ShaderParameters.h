//
// Created by HUSTLX on 2024/10/19.
//

#ifndef HARDWAREPATHTRACER_SHADERPARAMETERS_H
#define HARDWAREPATHTRACER_SHADERPARAMETERS_H

#include "core/Core.h"
#include <unordered_map>
#include <unordered_set>
#include <any>
#include <map>
#include <set>
#include "RenderPassCommon.h"


namespace HWPT {
    enum class ShaderParameterType : uint8_t {
        None = 0x0,
        Float,
        Float2,
        Float3,
        Float4,
        Int,
        Int2,
        Int3,
        Int4,
        UInt,
        UInt2,
        UInt3,
        UInt4,
        Texture2D,
        RWTexture2D,
        UniformBuffer,
        StorageBuffer,
        RWStorageBuffer,
        Buffer = StorageBuffer,
        RWBuffer = RWStorageBuffer,
        StructuredBuffer = StorageBuffer,
        RWStructuredBuffer = RWStorageBuffer
    };

    struct ShaderParameterMetaData {
        std::string ParameterName;
        ShaderParameterType ParameterType = ShaderParameterType::None;
        void* Parameter = nullptr;
        VkWriteDescriptorSet ParameterDescriptorSet{};
    };

    class ShaderParameters {
    public:
        friend class RenderPassBase;

        explicit ShaderParameters(RenderPassBase* RenderPass);

        virtual ~ShaderParameters();

        void AddParameters(const std::initializer_list<std::pair<std::string, ShaderParameterType>>& InitList);

        void AddParameter(const std::string& ParameterName, ShaderParameterType Type);

        // TODO: Manage Parameter LiftCycle in RenderGraph rather than RenderPass
        template<typename ParameterType>
        void SetParameter(const std::string& Name, const ParameterType& Parameter) {
            auto It = m_parameterMetaData.find(Name);
            if (It == m_parameterMetaData.end()) {
                throw std::runtime_error("Trying to retrieve parameter that doesn't exist");
            }

            auto &MetaData = It->second;
            MetaData.Parameter = Parameter;
        }

        template<typename RetType>
        auto GetParameter(const std::string& Name) -> RetType& {
            auto It = m_parameterMetaData.find(Name);
            if (It == m_parameterMetaData.end()) {
                throw std::runtime_error("Trying to retrieve parameter that doesn't exist");
            }

            auto &MetaData = It->second;
            return std::any_cast<RetType>(MetaData.Parameter);
        }

        void OnShaderParameterSetFinish();

        auto GetDescriptorSetLayout() -> VkDescriptorSetLayout& {
            return m_descriptorSetLayout;
        }

        auto GetDescriptorSets() -> VkDescriptorSet& {
            return m_descriptorSet;
        }

        void OnRenderPassBegin(uint Index = 0);

    private:
        void CreateDescriptorSetLayout();

        void CreateDescriptorSets();

//        void PushConstants(VkCommandBuffer CommandBuffer);

//        template<uint ConstantSize>
//        void SetConstant(VkCommandBuffer CommandBuffer, ShaderParameterMetaData& MetaData);

    private:
        std::unordered_map<std::string, ShaderParameterMetaData> m_parameterMetaData;
        RenderPassBase* m_renderPass = nullptr;
        VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
        bool m_parameterSetFinish = false;
        VkDescriptorSet m_descriptorSet;
        bool m_descriptorSetsAllocated = false;
    };
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_SHADERPARAMETERS_H
