cmake_minimum_required(VERSION 3.20)

include("cmake/prelude.cmake")

configure_main_project("notarius" "") # blank to aquire/assign version from the git repo
                                      # yy.mm.dd is used if the repo does not have a 
                                      # a tag (e.g., a tag of 'v1.0.0' will be transformed
                                      # to '1.0.0', etc.
                                      
project(
   ${PROJECT_NAME}
   VERSION "${notarius_PROJECT_VERSION}"
   LANGUAGES CXX
)

# See: config.cmake for details.
#
generate_notarius()