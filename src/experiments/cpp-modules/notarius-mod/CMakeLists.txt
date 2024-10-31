# Note:\n CMake 3.28 or higher is required to support C++ 20 modules.
#
cmake_minimum_required(VERSION 3.28)

project(notarius-mod LANGUAGES CXX)

include("./cmake/prologue.cmake")

fetch_and_configure_dependencies()

make_executable("src/notarius-test.cpp" "notarius")
