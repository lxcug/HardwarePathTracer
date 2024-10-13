cmake_minimum_required(VERSION 3.27)
project(HardwarePathTracer)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_CLANG_TIDY "clang-tidy")

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)

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
)
