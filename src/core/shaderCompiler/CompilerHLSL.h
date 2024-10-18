//
// Created by HUSTLX on 2024/10/18.
//

#ifndef HARDWAREPATHTRACER_COMPILERHLSL_H
#define HARDWAREPATHTRACER_COMPILERHLSL_H

#include "core/Core.h"
#include <filesystem>
#include <cstdlib>
#include <array>
#include "core/shader/ShaderBase.h"
#include "Windows.h"


namespace HWPT {
    class HLSLCompiler {
    public:

        static void CompileShader(const std::string& ShaderFile, const std::string& Entry,
                                  ShaderType Type, const std::string& OutFile);

        static auto ExecCmd(const std::string& cmd) -> std::string;

    private:
        inline static std::filesystem::path s_dxcPath = "../../vendor/dxc_2024_07_31";
        inline static std::filesystem::path s_hlslDirectory = "../../shader/HLSL";
    };
}  // namespace HWPT


#endif //HARDWAREPATHTRACER_COMPILERHLSL_H
