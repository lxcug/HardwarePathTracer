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
        ShaderBase(ShaderType ShaderType, const std::filesystem::path& ShaderPath, const char* Entry);

        virtual ~ShaderBase();

        void CreateShaderModule(const std::filesystem::path& ShaderPath);

        void CreateShaderModule(const std::vector<char>& ShaderSource);

        auto GetHandle() -> VkShaderModule {
            return m_shaderModule;
        }

        [[nodiscard]] auto GetEntryName() const -> const char* {
            return m_entryName;
        }

//        void BindShaderStage(const std::string& Entry);

    private:
        VkShaderModule m_shaderModule = VK_NULL_HANDLE;
        ShaderType m_shaderType = ShaderType::None;
        const char* m_entryName;
    };


    static auto LoadShaderFile(const std::filesystem::path& ShaderPath) -> std::vector<char>;
}  // namespace HWPT

#endif //HARDWAREPATHTRACER_SHADERBASE_H
