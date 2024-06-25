# ---- In-source guard ----
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(
      FATAL_ERROR
      "In-source builds are not supported. "
      "Please read the BUILDING document before trying to build this project. "
      "You may need to delete 'CMakeCache.txt' and 'CMakeFiles/' first."
  )
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(${PROJECT_NAME}_STANDALONE ON)
else()
    set(${PROJECT_NAME}_STANDALONE OFF)
endif()

set(${PROJECT_NAME}_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_EXPORT_COMPILE_COMMANDS "on")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
   list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
   list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
endif()

include("${CMAKE_DIR}/core/project-is-top-level.cmake")
include("${CMAKE_DIR}/core/variables.cmake")
  
# sets '${PROJECT_NAME}_SOURCES_DIR' variable (CACHE INTERNAL)
#
function (set_projects_sources_directory)
   #
   # Supports the different conventions teams like to use.
   #
   # First path found is the 'default' used. Therefore results are 
   # mutally exclusive!
   #
   set(${PROJECT_NAME}_SOURCES_DIR "")  # Initialize ${PROJECT_NAME}_SOURCES_DIR variable
   
   set(PROJECTS_DIRS
       "${CMAKE_CURRENT_SOURCE_DIR}/projects"
       "${CMAKE_CURRENT_SOURCE_DIR}/sources"
       "${CMAKE_CURRENT_SOURCE_DIR}/source"
       "${CMAKE_CURRENT_SOURCE_DIR}/src"
       "${CMAKE_CURRENT_SOURCE_DIR}"
   )
   
   foreach(dir ${PROJECTS_DIRS})
       if (NOT EXISTS "${${PROJECT_NAME}_SOURCES_DIR}" AND EXISTS "${dir}")
           set(${PROJECT_NAME}_SOURCES_DIR ${dir} CACHE INTERNAL "" FORCE)
           set(TOP_LEVEL_PROJECT_SRC_DIR ${dir} CACHE INTERNAL "" FORCE)
           return()
       endif()
   endforeach()
   
endfunction()

set_projects_sources_directory()

include("${CMAKE_DIR}/core/utils.cmake")

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include")
   set(${PROJECT_NAME}_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
   set_common_include_directory("${CMAKE_CURRENT_SOURCE_DIR}/include" "")
endif()

message(STATUS "\n${PROJECT_NAME} Manifest:")
message(STATUS " ${PROJECT_NAME}_PROJECT_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS " ${PROJECT_NAME}_SOURCES_DIR: ${${PROJECT_NAME}_SOURCES_DIR}")
message(STATUS " ${PROJECT_NAME}_INCLUDE_DIR: ${${PROJECT_NAME}_INCLUDE_DIR}")
message(STATUS " ${PROJECT_NAME}_STANDALONE : ${${PROJECT_NAME}_STANDALONE}\n")

if (EXISTS "${CMAKE_DIR}/config.cmake")
   include("${CMAKE_DIR}/config.cmake")
else()
    message(FATAL_ERROR "Required cmake module, '${CMAKE_DIR}/config.cmake', is missing!")
endif()

function (check_for_pkg_config_executable)

   if (NOT EXISTS ${PKG_CONFIG_EXECUTABLE})
       message("-------------------------------------------------------------------------------------------------")
       message("'pkg-config' executable not found.")
       message(STATUS "\nSome third-dependencies may be ignored resulting in less optimal performance."
                      "\nTo install pkg-config, follow the instructions for your operating system:")

       if (UNIX)
         message(STATUS "Ubuntu/Debian:"
                        "\n  sudo apt update"
                        "\n  sudo apt install pkg-config"
                        "\n"
                        "\nFedora:"
                        "\n  sudo dnf install pkg-config"
                        "\n"
                        "\nArch Linux:"
                        "\n  sudo pacman -S pkgconf"
                        "\n"
                        "\nmacOS:"
                        "\n  brew install pkg-config")
       elseif (WIN32)
         message(STATUS "\nWindows:"
                        "\n  Follow the instructions for installing pkg-config on Windows using MSYS2 or other methods."
                        "\n  Example:"
                        "\n  1. Install MSYS2 from https://www.msys2.org/"
                        "\n  2. Open MSYS2 MinGW 64-bit shell"
                        "\n  3. Run: pacman -Syu"
                        "\n  4. Run: pacman -S mingw-w64-x86_64-pkg-config"
                        "\n"
                        "\n  After installation, set PKG_CONFIG_EXECUTABLE in your CMake command (E.g.):"
                        "\n  cmake -DCMAKE_PREFIX_PATH=\"C:/msys64/mingw64\" -DPKG_CONFIG_EXECUTABLE=\"C:/msys64/mingw64/bin/pkg-config.exe\" path/to/your/project\n")

         message("-------------------------------------------------------------------------------------------------")   
       endif()
   endif()
   
endfunction()

