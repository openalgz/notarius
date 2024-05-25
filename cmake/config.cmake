set(CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Your Main Project Configuration Options:
option(@ALWAYS_FETCH_DEPENDENCIES "Always Fetch Dependencies." ON)
option(@SUPRESS_COMPILER_WARNING "Suppress compiler warnings when building." ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ROOT_REPO_DIRECTORY "${CMAKE_SOURCE_DIR}" CACHE INTERNAL "" FORCE)
set(PROJECT_DESCRIPTION "Notarius C++ Fast Logging Library" CACHE INTERNAL "")
set(PROJECT_URL "https://github.com/openalgz/notarius" CACHE INTERNAL "")

# Set Dependencies Here:
#
list(APPEND PROJECT_DEPENDENCIES_LIST "spdlog, https://github.com/gabime/spdlog.git, v1.13.0")
list(APPEND PROJECT_DEPENDENCIES_LIST "ut,     https://github.com/boost-ext/ut.git,  v2.0.1")

macro(generate_notarius)

   if (PROJECT_IS_TOP_LEVEL)
      configure_boost_micro_unit_testing()
      fetch_content_and_make_available("${PROJECT_DEPENDENCIES_LIST}")   
      include("${CMAKE_DIR}/core/dev-mode.cmake")
   else()
      make_header_only_project()
   endif()
   
endmacro()

macro(suppress_compiler_warnings)
   if(MSVC)
      # Suppress warning C4996 (deprecated functions)
      add_compile_options(/wd4996)
   endif()
   add_compile_options("-Wno-#pragma-messages" "-Wbraced-scalar-init")
endmacro()

if (@SUPRESS_COMPILER_WARNING)
   suppress_compiler_warnings()
endif()

# if version is empty then git version is used.
#
macro(configure_main_project name version)

   set(PRODUCT_NAME ${name})

   if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include")
      set(${PROJECT_NAME}_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE INTERNAL "" FORCE)
      set(include_dir "${${PROJECT_NAME}_INCLUDE_DIR}")
      set_common_include_directory("${include_dir}" "")
   endif()

   if(NOT DEFINED version OR "${version}" STREQUAL "")
      generate_git_version_include("${name}" "${PROJECT_URL}" "${PROJECT_DESCRIPTION}" "${include_dir}/${name}/version.in.hpp")
      # assuming 'v' in the tag.
      string(SUBSTRING "${MY_PROJECT_VERSION}" 1 -1 GIT_VERSION_TAG)
      string(SUBSTRING "${GIT_VERSION_TAG}" 0 1 first_char)
      if(first_char STREQUAL ".")
         # First char cannot be '.'
         string(SUBSTRING "${GIT_VERSION_TAG}" 1 -1 GIT_VERSION_TAG)
      endif()
      set("${name}_PROJECT_VERSION" "${GIT_VERSION_TAG}")
   else()
      force_project_version("${version}" "${name}" "${PROJECT_URL}" "${PROJECT_DESCRIPTION}" "${include_dir}/${name}/version.in.hpp")
   endif()

endmacro()


 