macro(is_topmost_project)
    # This variable is set by project() in CMake 3.21+
    if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        set(PROJECT_IS_TOP_LEVEL ON CACHE INTERNAL "Indicates if this is the top-level project" FORCE)
    else()
        set(PROJECT_IS_TOP_LEVEL OFF CACHE INTERNAL "Indicates if this is the top-level project" FORCE)
    endif()
endmacro()

# Always Call
is_topmost_project()