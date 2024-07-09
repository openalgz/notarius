cmake_minimum_required(VERSION 3.20)

project("write-tests" LANGUAGES CXX)

make_project_executable("write-tests" write-tests.cpp)

enable_testing()

add_test(NAME "write-tests" COMMAND "write-tests")

