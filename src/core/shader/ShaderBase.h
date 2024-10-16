//
// Created by HUSTLX on 2024/10/8.
//

#ifndef HARDWAREPATHTRACER_SHADERBASE_H
#define HARDWAREPATHTRACER_SHADERBASE_H

#include "core/Core.h"
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>


namespace HWPT {
    enum class ShaderType : uint8_t {
        None = 0x0,
        Vertex,
        Geometry,
        Fragment,
        Compute
    };

    class ShaderBase {
    public:
        ShaderBase(ShaderType InShaderType, const std::filesystem::path& ShaderPath);

        virtual ~ShaderBase();

        void CreateShaderModule(const std::filesystem::path& ShaderPath);

        void CreateShaderModule(const std::vector<char>& ShaderSource);

        auto GetHandle() -> VkShaderModule {
            return m_shaderModule;
        }

//        void BindShaderStage(const std::string& Entry);

    private:
        VkShaderModule m_shaderModule = VK_NULL_HANDLE;
        ShaderType m_shaderType = ShaderType::None;
    };


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

#endif //HARDWAREPATHTRACER_SHADERBASE_H
