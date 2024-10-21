cmake_minimum_required(VERSION 3.27)
project(HardwarePathTracer)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_CLANG_TIDY "clang-tidy")

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)

add_subdirectory(${PROJECT_SOURCE_DIR}/vendor)

add_executable(HardwarePathTracer src/core/PTMain.cpp
        src/core/application/Application.cpp
        src/core/application/Application.h
        src/core/application/VulkanBackendApp.cpp
        src/core/application/VulkanBackendApp.h
        src/core/Core.h
        src/core/buffer/VertexBuffer.cpp
        src/core/buffer/VertexBuffer.h
        src/core/buffer/IndexBuffer.cpp
        src/core/buffer/IndexBuffer.h
        src/core/RHI.cpp
        src/core/RHI.h
        src/core/shader/ShaderBase.h
        src/core/shader/ShaderBase.cpp
        src/core/shader/ComputeShader.cpp
        src/core/shader/ComputeShader.h
        src/core/buffer/StorageBuffer.cpp
        src/core/buffer/StorageBuffer.h
        src/core/buffer/UniformBuffer.cpp
        src/core/buffer/UniformBuffer.h
        src/core/texture/Texture2D.cpp
        src/core/texture/Texture2D.h
        src/core/texture/TextureShared.cpp
        src/core/texture/TextureShared.h
        src/core/texture/Sampler.cpp
        src/core/texture/Sampler.h
        src/core/FPSCalculator.cpp
        src/core/FPSCalculator.h
        src/core/application/ImGuiIntegration.cpp
        src/core/application/ImGuiIntegration.h
        src/core/Model.cpp
        src/core/Model.h
        src/core/buffer/VertexBufferLayout.cpp
        src/core/buffer/VertexBufferLayout.h
        src/core/renderGraph/RenderGraph.cpp
        src/core/renderGraph/RenderGraph.h
        src/core/renderGraph/RenderPassCommon.cpp
        src/core/renderGraph/RenderPassCommon.h
        src/core/renderGraph/RasterPass.cpp
        src/core/renderGraph/RasterPass.h
        src/core/renderGraph/ComputePass.cpp
        src/core/renderGraph/ComputePass.h
        src/core/renderGraph/ShaderParameters.cpp
        src/core/renderGraph/ShaderParameters.h
        src/core/buffer/FrameBuffer.cpp
        src/core/buffer/FrameBuffer.h
        src/core/SwapChain.cpp
        src/core/SwapChain.h
)

include_directories(
        ${PROJECT_SOURCE_DIR}/src
)

target_include_directories(
        HardwarePathTracer
        PRIVATE ./include
        PRIVATE ${Stb_INCLUDE_DIR}
)

target_link_libraries(
        HardwarePathTracer
        PRIVATE Vulkan::Vulkan
        PRIVATE glfw
        PRIVATE glm::glm
        PRIVATE imgui
        PRIVATE tinyobjloader::tinyobjloader
)

add_executable(
        CompileHLSL src/core/shaderCompiler/CompilerHLSL.cpp
)

target_link_libraries(
        CompileHLSL
        PRIVATE Vulkan::Vulkan
        PRIVATE glfw
        PRIVATE glm::glm
        PRIVATE imgui
        PRIVATE tinyobjloader::tinyobjloader
)
