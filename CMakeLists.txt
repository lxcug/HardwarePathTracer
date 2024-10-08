cmake_minimum_required(VERSION 3.27)
project(HardwarePathTracer)

set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
#find_package(Stb REQUIRED)

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
)

include_directories(
        ${PROJECT_SOURCE_DIR}/src
)

target_include_directories(
        HardwarePathTracer
        PRIVATE ./include
#        PRIVATE ${Stb_INCLUDE_DIR}
)

target_link_libraries(
        HardwarePathTracer
        PRIVATE Vulkan::Vulkan
        PRIVATE glfw
        PRIVATE glm::glm
)
