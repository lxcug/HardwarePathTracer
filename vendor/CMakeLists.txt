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
