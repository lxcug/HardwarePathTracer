set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)


if (NOT TARGET imgui)
    include(imgui.cmake)
endif ()
