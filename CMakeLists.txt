cmake_minimum_required(VERSION 3.24)

include("cmake/core/prelude.cmake")

configure_projects("notarius" "") # blank to aquire/assign version from the git repo

project(
   ${PRODUCT_NAME}
   VERSION ${PRODUCT_VERSION}
   LANGUAGES CXX
)

# See: config.cmake for details.
#
configure_notarius()