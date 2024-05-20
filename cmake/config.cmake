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

macro(configure_notarius)

   if (PROJECT_IS_TOP_LEVEL)
      configure_boost_micro_unit_testing()
      fetch_content_and_make_available("${PROJECT_DEPENDENCIES_LIST}")   
      include("cmake/core/dev-mode.cmake")
   else()
      make_header_only_project()
   endif()
   
endmacro()

macro(suppress_compiler_warnings)
   if(MSVC)
      # Suppress warning C4996 (deprecated functions)
      add_compile_options(/wd4996)
   endif()
endmacro()

if (@SUPRESS_COMPILER_WARNING)
   suppress_compiler_warnings()
endif()


 