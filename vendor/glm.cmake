set(glm_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glm)


file(
        GLOB_RECURSE glm_SOURCES
        ${glm_SOURCE_DIR}/glm/*.cpp
)

add_library(
        glm STATIC
        ${glm_SOURCES}
)

target_compile_definitions(glm PUBLIC GLM_ENABLE_EXPERIMENTAL)

target_include_directories(
        glm
        PUBLIC ${glm_SOURCE_DIR}
)
