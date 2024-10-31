set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(@ALWAYS_FETCH_DEPENDENCIES "Always fetch dependencies even if they exist locally." OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (e.g., Debug, Release)." FORCE)
endif()

option(BUILD_DEBUG "Build in debug mode" OFF)
if(BUILD_DEBUG)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Debug Build")
endif()

option(@VERBOSE_OUTPUT "Enable verbose output during the build process" OFF)

if(@VERBOSE_OUTPUT)
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/W4 /WX /Od /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4003 /wd5109 /Zc:preprocessor /Wv:18 /Zc:__cplusplus")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -Wpedantic -Werror -O0 -g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

option(ENABLE_TESTING "Enable testing" ON)

set(CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake" CACHE PATH "Path to ${PROJECT_NAME} CMake modules directory" FORCE)

if (CMAKE_VERBOSE_MAKEFILE)
   get_filename_component(CURRENT_MODULE_NAME "${CMAKE_CURRENT_LIST_FILE}" NAME)
   message(STATUS "- importing ${CURRENT_MODULE_NAME}.")
endif()

include("${CMAKE_DIR}/Dependencies.cmake")


