# ---- In-source guard ----
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(
      FATAL_ERROR
      "In-source builds are not supported. "
      "Please read the BUILDING document before trying to build this project. "
      "You may need to delete 'CMakeCache.txt' and 'CMakeFiles/' first."
  )
endif()

set(PROJECT_SOURCE_DIR "${CMAKE_SOURCE_DIR}" CACHE INTERNAL "" FORCE)

if (NOT EXISTS "${CMAKE_DIR}" AND EXISTS "${CMAKE_SOURCE_DIR}/cmake")
   set(CMAKE_DIR "${CMAKE_SOURCE_DIR}/cmake" CACHE INTERNAL "" FORCE) 
   list(APPEND CMAKE_MODULE_PATH "${CMAKE_DIR}")
endif()

include("${CMAKE_DIR}/core/project-is-top-level.cmake")
include("${CMAKE_DIR}/core/variables.cmake")
  
set(ROOT_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "" FORCE)

if (EXISTS "${ROOT_PROJECT_DIR}/include")
   set(ROOT_INCLUDE_DIR "${ROOT_PROJECT_DIR}/include" CACHE INTERNAL "" FORCE)
endif()

# sets 'PROJECTS_SOURCES_DIR' variable (CACHE INTERNAL)
#
function (set_projects_sources_directory)
   #
   # Supports the different conventions teams like to use.
   #
   # First path found is the 'default' used. Therefore results are 
   # mutally exclusive!
   #
   set(PROJECTS_SOURCES_DIR "")  # Initialize PROJECTS_SOURCES_DIR variable
   set(PROJECTS_DIRS
       "${CMAKE_CURRENT_SOURCE_DIR}/projects"
       "${CMAKE_CURRENT_SOURCE_DIR}/sources"
       "${CMAKE_CURRENT_SOURCE_DIR}/source"
       "${CMAKE_CURRENT_SOURCE_DIR}/src"
       "${CMAKE_CURRENT_SOURCE_DIR}"
   )
   
   foreach(dir ${PROJECTS_DIRS})
       if (NOT EXISTS "${PROJECTS_SOURCES_DIR}" AND EXISTS "${dir}")
           set(PROJECTS_SOURCES_DIR ${dir} CACHE INTERNAL "" FORCE)
           return()
       endif()
   endforeach()
   
endfunction()

set_projects_sources_directory()

include("${CMAKE_DIR}/core/utils.cmake")

message(STATUS "\n${CMAKE_PROJECT_NAME} Manifest:")
message(STATUS " PROJECTS_DIR . . . . ${PROJECTS_DIR}")
message(STATUS " ROOT_CMAKE_DIR . . . ${CMAKE_DIR}")
message(STATUS " ROOT_PROJECT_DIR . . ${ROOT_PROJECT_DIR}")
message(STATUS " ROOT_INCLUDE_DIR . . ${ROOT_INCLUDE_DIR}")
message(STATUS " IS_TOPLEVEL_PROJECT: ${IS_TOPLEVEL_PROJECT}\n")

set_common_include_directory("${ROOT_INCLUDE_DIR}" "called from prelude.cmake: ln:${CMAKE_CURRENT_LIST_LINE}")

# if version is empty then git version is used.
#
macro(configure_projects name version)

   set(PRODUCT_NAME ${name})
   
   if(NOT DEFINED version OR "${version}" STREQUAL "")
      set_project_info_by_git("${name}" "${PROJECT_URL}" "${PROJECT_DESCRIPTION}" "${ROOT_INCLUDE_DIR}/${name}/version.h.in")
      string(SUBSTRING ${GIT_VERSION_TAG} 1 -1 PRODUCT_VERSION) 
   else()
      force_project_version("${version}" "${name}" "${PROJECT_URL}" "${PROJECT_DESCRIPTION}" "${ROOT_INCLUDE_DIR}/${name}/version.h.in")
   endif()
   
endmacro()

if (EXISTS "${CMAKE_DIR}/config.cmake")
   include("${CMAKE_DIR}/config.cmake")
endif()