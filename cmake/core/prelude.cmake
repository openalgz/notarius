set(PROJECT_NAME "notarius")

set(CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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
           return()
       endif()
   endforeach()
   
endfunction()

set_projects_sources_directory()

include("${CMAKE_DIR}/core/utils.cmake")

message(STATUS "\n${PROJECT_NAME} Manifest:")
message(STATUS " ${PROJECT_NAME}_PROJECT_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS " ${PROJECT_NAME}_SOURCES_DIR: ${${PROJECT_NAME}_SOURCES_DIR}")
message(STATUS " ${PROJECT_NAME}_INCLUDE_DIR: ${${PROJECT_NAME}_INCLUDE_DIR}")
message(STATUS " ${PROJECT_NAME}_STANDALONE : ${${PROJECT_NAME}_STANDALONE}\n")

if (EXISTS "${CMAKE_DIR}/config.cmake")
   include("${CMAKE_DIR}/config.cmake")
endif()
