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
    enum ShaderType {
        None = 0x0,
        Vertex,
        Geometry,
        Fragment,
        Compute
    };

    class ShaderBase {
    public:
        virtual ~ShaderBase();

        void CreateShaderModule(const std::filesystem::path& ShaderPath);

        void CreateShaderModule(const std::vector<char>& ShaderSource);

//        void BindShaderStage(const std::string& Entry);

    public:
        VkShaderModule m_shaderModule = VK_NULL_HANDLE;
        ShaderType m_shaderType = ShaderType::None;
    };


    static std::vector<char> LoadShaderFile(const std::filesystem::path& shaderPath) {
        std::ifstream shaderFile(shaderPath, std::ios::ate | std::ios::binary);

        if (!shaderFile.is_open()) {
            throw std::runtime_error("Failed to open file!");
        }

        size_t fileSize = shaderFile.tellg();
        std::vector<char> buffer(fileSize);
        shaderFile.seekg(0);
        shaderFile.read(buffer.data(), (long long) fileSize);
        shaderFile.close();

        return buffer;
    }
}

#endif //HARDWAREPATHTRACER_SHADERBASE_H
