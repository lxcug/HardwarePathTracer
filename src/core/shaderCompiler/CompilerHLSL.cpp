//
// Created by HUSTLX on 2024/10/18.
//

#include "CompilerHLSL.h"


namespace HWPT {

    void HLSLCompiler::CompileShader(const std::string &ShaderFile, const std::string &Entry,
                                     ShaderType Type, const std::string &OutFile) {
        std::string ShaderStageString;
        switch (Type) {
            case ShaderType::Vertex:
                ShaderStageString = "vs_6_0";
                break;
            case ShaderType::Fragment:
                ShaderStageString = "ps_6_0";
                break;
            case ShaderType::Geometry:
                ShaderStageString = "gs_6_0";
                break;
            case ShaderType::Compute:
                ShaderStageString = "cs_6_0";
                break;
        }
        std::string CompileCommand =
                absolute(s_dxcPath).string() + "/bin/x64/dxc.exe" +
                " -E " + Entry +
                " -T " + ShaderStageString +
                " -spirv" +
                " -Fo " + absolute(s_hlslDirectory).string() + "/" + OutFile + ".spv " +
                absolute(s_hlslDirectory).string() + "/" + ShaderFile;

        auto RetString = HLSLCompiler::ExecCmd(CompileCommand);

        std::cout << RetString << "\n";
        std::cout.flush();
    }

    auto HLSLCompiler::ExecCmd(const std::string &cmd) -> std::string {
        HANDLE hStdOutRead = NULL;
        HANDLE hStdOutWrite = NULL;
        SECURITY_ATTRIBUTES saAttr;

        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &saAttr, 0)) {
            std::cerr << "CreatePipe failed\n";
            return "";
        }

        if (!SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0)) {
            std::cerr << "SetHandleInformation failed\n";
            return "";
        }

        STARTUPINFOA siStartInfo;
        PROCESS_INFORMATION piProcInfo;
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));

        siStartInfo.cb = sizeof(STARTUPINFOA);
        siStartInfo.hStdError = hStdOutWrite;
        siStartInfo.hStdOutput = hStdOutWrite;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        if (!CreateProcessA(NULL,
                            const_cast<char *>(cmd.c_str()),
                            NULL,
                            NULL,
                            TRUE,
                            0,
                            NULL,
                            NULL,
                            &siStartInfo,
                            &piProcInfo)) {
            std::cerr << "CreateProcess failed (" << GetLastError() << ")\n";
            return "";
        }

        CloseHandle(hStdOutWrite);

        std::string output;
        DWORD dwRead;
        CHAR chBuf[4096];
        BOOL bSuccess = FALSE;
        for (;;) {
            bSuccess = ReadFile(hStdOutRead, chBuf, sizeof(chBuf) - 1, &dwRead, NULL);
            if (!bSuccess || dwRead == 0) break;
            chBuf[dwRead] = '\0';
            output += chBuf;
        }

        CloseHandle(hStdOutRead);
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);

        return output;
    }
}  // namespace HWPT


auto main() -> int {
    HWPT::HLSLCompiler::CompileShader("Mesh.hlsl", "VSMain", HWPT::ShaderType::Vertex, "Vert");
    HWPT::HLSLCompiler::CompileShader("Mesh.hlsl", "PSMain", HWPT::ShaderType::Fragment, "Frag");
    HWPT::HLSLCompiler::CompileShader("Particle.hlsl", "VSMain", HWPT::ShaderType::Vertex, "ParticleVert");
//    HWPT::HLSLCompiler::CompileShader("Particle.hlsl", "GSMain", HWPT::ShaderType::Geometry, "ParticleGeometry");
    HWPT::HLSLCompiler::CompileShader("Particle.hlsl", "PSMain", HWPT::ShaderType::Fragment, "ParticleFrag");
    HWPT::HLSLCompiler::CompileShader("UpdateParticle.hlsl", "UpdateParticles", HWPT::ShaderType::Compute, "UpdateParticle");

    return 0;
}
