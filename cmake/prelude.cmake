# Prelude must be called prior to calling CMake 'project',
# therefore your main project name must be defined here.
#
set(PROJECT_NAME "notarius")

set(CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include ("${CMAKE_DIR}/core/prelude_impl.cmake")
