cmake_minimum_required(VERSION 3.24)

include("cmake/prelude.cmake")

configure_main_project("demo" "") # blank to aquire/assign version from the git repo
                                  # yy.mm.dd is used if the repo does not have a 
                                  # a tag (e.g., a tag of 'v1.0.0' will be transformed
                                  # to '1.0.0', etc.                             
project(
   ${PROJECT_NAME}
   VERSION "${MY_PROJECT_VERSION}"
   LANGUAGES CXX
)

# See: config.cmake for details.
#
generate_demo()
