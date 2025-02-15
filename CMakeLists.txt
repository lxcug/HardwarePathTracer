cmake_minimum_required(VERSION 3.27)
project(Shadowy)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_CLANG_TIDY "clang-tidy")

find_package(Vulkan REQUIRED)

add_subdirectory(${PROJECT_SOURCE_DIR}/vendor)

file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

MESSAGE(STATUS ${SOURCES})

add_executable(Shadowy
        ${SOURCES}
        ${stb_SOURCES}
        src/core/post_processing/AccumulationPass.cpp
        src/core/post_processing/AccumulationPass.h
)

target_include_directories(
        Shadowy
        PUBLIC ${PROJECT_SOURCE_DIR}/src
        PUBLIC ${PROJECT_SOURCE_DIR}/vendor/stb
)

target_link_libraries(
        Shadowy
        PUBLIC Vulkan::Vulkan
        PUBLIC glfw
        PUBLIC glm
        PUBLIC imgui
        PUBLIC tinyobjloader
        PUBLIC ImGuiFileDialog
        PUBLIC NRI
        PUBLIC NRD
        PUBLIC NRDIntegration
)
