set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)


if (NOT TARGET imgui)
    include(imgui.cmake)
endif ()

if (NOT TARGET ImGuiFileDialog)
    add_subdirectory(ImGuiFileDialog)
endif ()

if (NOT TARGET stb)
    include(stb.cmake)
endif ()

if (NOT TARGET tinyobjloader)
    add_subdirectory(tinyobjloader)
endif ()

if (NOT TARGET glm)
    include(glm.cmake)
endif ()

if (NOT TARGET glfw)
    add_subdirectory(glfw)
endif ()

if (NOT TARGET NRI)
    set(NRI_STATIC_LIBRARY ON)
    add_subdirectory(nri)
endif ()

if (NOT TARGET NRD AND NOT TARGET NRDIntegration)
    set(NRD_STATIC_LIBRARY ON)
    add_subdirectory(NRD)
endif ()

if (NOT TARGET assimp)
    add_subdirectory(assimp)
endif ()
