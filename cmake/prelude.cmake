# Prelude must be called prior to calling CMake 'project',
# therefore your main project name must be defined here.
#
set(PROJECT_NAME "notarius")

set(CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include ("${CMAKE_DIR}/core/prelude_impl.cmake")

function (query_installed_packages)

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
