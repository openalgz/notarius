set(CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Your Main Project Configuration Options:
option(@ALWAYS_FETCH_DEPENDENCIES "Always Fetch Dependencies." ON)
option(@SUPRESS_COMPILER_WARNINGS "Suppress compiler warnings when building." ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ROOT_REPO_DIRECTORY "${CMAKE_SOURCE_DIR}" CACHE INTERNAL "" FORCE)
set(PROJECT_DESCRIPTION "Notarius ${PROJECT_NAME} Project: Using the C++ Fast Logging Library" CACHE INTERNAL "")
set(PROJECT_URL "https://github.com/openalgz/notarius" CACHE INTERNAL "")

# Set Dependencies Here:
#
macro(generate_demo)
   #
   # Get notarius...you may append other dependencies here also...
   #
   list(APPEND PROJECT_DEPENDENCIES_LIST "notarius, https://github.com/openalgz/notarius.git, main")
   list(APPEND PROJECT_DEPENDENCIES_LIST "ut, https://github.com/openalgz/ut.git, main")

   fetch_content_and_make_available("${PROJECT_DEPENDENCIES_LIST}")   
   
   include("${CMAKE_DIR}/core/dev-mode.cmake")

endmacro()

macro(suppress_compiler_warnings)
   if (MSVC)
      add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS)
   elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
      message("")
   endif()
endmacro()

if (@SUPRESS_COMPILER_WARNINGS)
   suppress_compiler_warnings()
endif()

# if version is empty then git version is used.
#
macro(configure_main_project name version)

   if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include")
      set(${PROJECT_NAME}_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE INTERNAL "" FORCE)
      set(include_dir "${${PROJECT_NAME}_INCLUDE_DIR}")
      set_common_include_directory("${include_dir}" "")
   endif()

   if(NOT DEFINED version OR "${version}" STREQUAL "")
      generate_git_version_include("${name}" "${PROJECT_URL}" "${PROJECT_DESCRIPTION}" "${include_dir}/${name}/version.in.hpp")
      string(REGEX REPLACE "^v\\.?" "" MY_PROJECT_VERSION "${MY_PROJECT_VERSION}")
      set("${name}_PROJECT_VERSION" "${GIT_VERSION_TAG}")
   else()
      force_project_version("${version}" "${name}" "${PROJECT_URL}" "${PROJECT_DESCRIPTION}" "${include_dir}/${name}/version.in.hpp")
   endif()

endmacro()


 