//
// Created by HUSTLX on 2024/10/8.
//

#include "ShaderBase.h"
#include "core/application/VulkanBackendApp.h"


namespace HWPT {


    ShaderBase::ShaderBase(ShaderType ShaderType, const std::filesystem::path &ShaderPath,
                           const char *Entry)
                           : m_shaderType(ShaderType), m_entryName(Entry) {
        CreateShaderModule(LoadShaderFile(ShaderPath));
    }

    ShaderBase::~ShaderBase() {
        vkDestroyShaderModule(GetVKDevice(), m_shaderModule, nullptr);
    }

    void ShaderBase::CreateShaderModule(const std::filesystem::path &ShaderPath) {
        CreateShaderModule(LoadShaderFile(ShaderPath));
    }

    void ShaderBase::CreateShaderModule(const std::vector<char> &ShaderSource) {
        Check(m_shaderType != ShaderType::None);
        VkShaderModuleCreateInfo CreateInfo{};
        CreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        CreateInfo.codeSize = ShaderSource.size();
        CreateInfo.pCode = reinterpret_cast<const uint32_t *>(ShaderSource.data());

        VK_CHECK(vkCreateShaderModule(GetVKDevice(), &CreateInfo, nullptr, &m_shaderModule));
    }

    static auto LoadShaderFile(const std::filesystem::path& ShaderPath) -> std::vector<char> {
        std::ifstream ShaderFile(ShaderPath, std::ios::ate | std::ios::binary);

        if (!ShaderFile.is_open()) {
            throw std::runtime_error("Failed to open file!");
        }

        size_t fileSize = ShaderFile.tellg();
        std::vector<char> Buffer(fileSize);
        ShaderFile.seekg(0);
        ShaderFile.read(Buffer.data(), static_cast<int64_t>(fileSize));
        ShaderFile.close();

        return Buffer;
    }
}  // namespace HWPT
