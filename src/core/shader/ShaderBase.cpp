//
// Created by HUSTLX on 2024/10/8.
//

#include "ShaderBase.h"
#include "core/application/VulkanBackendApp.h"


namespace HWPT {

    ShaderBase::~ShaderBase() {
        vkDestroyShaderModule(GetVKDevice(), m_shaderModule, nullptr);
    }

    void ShaderBase::CreateShaderModule(const std::filesystem::path &ShaderPath) {
        CreateShaderModule(LoadShaderFile(ShaderPath));
    }

    void ShaderBase::CreateShaderModule(const std::vector<char> &ShaderSource) {
        VkShaderModuleCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        CreateInfo.codeSize = ShaderSource.size();
        CreateInfo.pCode = reinterpret_cast<const uint32_t *>(ShaderSource.data());

        VK_CHECK(vkCreateShaderModule(GetVKDevice(), &CreateInfo, nullptr, &m_shaderModule));
    }

//    void ShaderBase::BindShaderStage(const std::string &Entry) {
//        VkPipelineShaderStageCreateInfo ShaderStageInfo{};
//
//        ShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//        switch (m_shaderType) {
//            case ShaderType::Vertex:
//                ShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
//                break;
//            case ShaderType::Geometry:
//                ShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
//                break;
//            case ShaderType::Fragment:
//                ShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//                break;
//            case ShaderType::Compute:
//                ShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
//                break;
//            default:
//                Check(false);
//        }
//        ShaderStageInfo.module = m_shaderModule;
//        ShaderStageInfo.pName = Entry.c_str();
//    }
}  // namespace HWPT
