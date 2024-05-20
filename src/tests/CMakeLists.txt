cmake_minimum_required(VERSION 3.27)

project("write-tests" LANGUAGES CXX)

add_definitions(-DSPDLOG_HEADER_ONLY)

make_project_executable("write-tests" write-tests.cpp)
