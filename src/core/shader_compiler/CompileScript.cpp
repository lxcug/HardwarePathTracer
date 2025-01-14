//
// Created by HUSTLX on 2025/1/5.
//

#include "CompilerHLSL.h"

auto main() -> int {
    HWPT::HLSLCompiler::CompileShader("Mesh.hlsl", "VSMain", HWPT::ShaderType::Vertex, "Vert");
    HWPT::HLSLCompiler::CompileShader("Mesh.hlsl", "PSMain", HWPT::ShaderType::Fragment, "Frag");
    HWPT::HLSLCompiler::CompileShader("Particle.hlsl", "VSMain", HWPT::ShaderType::Vertex, "ParticleVert");
//    HWPT::HLSLCompiler::CompileShader("Particle.hlsl", "GSMain", HWPT::ShaderType::Geometry, "ParticleGeometry");
    HWPT::HLSLCompiler::CompileShader("Particle.hlsl", "PSMain", HWPT::ShaderType::Fragment, "ParticleFrag");
    HWPT::HLSLCompiler::CompileShader("UpdateParticle.hlsl", "UpdateParticles", HWPT::ShaderType::Compute, "UpdateParticle");

    return 0;
}

