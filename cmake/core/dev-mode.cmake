enable_language(CXX)

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS YES)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(src/tests)
endif()

# Note: PROJECT_SOURCE_DIR is always the root directory of the project 
#       being configured. 

# Done in developer mode only, so users won't be bothered by this :)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}/*.hpp")
source_group(TREE "${PROJECT_SOURCE_DIR}/include" PREFIX headers FILES ${headers})

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/tests/*.cpp")
file(GLOB_RECURSE includes CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/tests/*.hpp")
source_group(TREE "${PROJECT_SOURCE_DIR}/src" PREFIX sources FILES ${sources} ${includes})

#[[ Debugging:
   message("\nSource files:")
   foreach(source IN LISTS sources)
       message(STATUS "  ${source}")
   endforeach()

   message("\nHeader files:")
   foreach(header IN LISTS headers)
       message(STATUS "  ${header}")
   endforeach()
   message("\n")
#]]

make_header_only_project()