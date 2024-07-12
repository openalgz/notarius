set(CMAKE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Your Main Project Configuration Options:
option(@ALWAYS_FETCH_DEPENDENCIES "Always Fetch Dependencies." ON)
option(@SUPRESS_COMPILER_WARNING "Suppress compiler warnings when building." ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# A demo project to start your own project with.
#
#[[
function(generate_demo_project)
   set(DEST_DIR ${CMAKE_SOURCE_DIR}/src/demo)
   file(COPY ${CMAKE_SOURCE_DIR}/scripts DESTINATION ${DEST_DIR})
   file(COPY ${CMAKE_SOURCE_DIR}/cmake DESTINATION ${DEST_DIR})
endfunction()
#]]

# Set Your Dependencies Here:
#
macro(generate_${PROJECT_NAME})

   if (PROJECT_IS_TOP_LEVEL)
   
      # deprecated: generate_demo_project()
   
      list(APPEND PROJECT_DEPENDENCIES_LIST "spdlog,  https://github.com/gabime/spdlog.git, v1.13.0")
      list(APPEND PROJECT_DEPENDENCIES_LIST "ut,      https://github.com/boost-ext/ut.git,  v2.0.1")
   
      configure_boost_micro_unit_testing()

      fetch_content_and_make_available("${PROJECT_DEPENDENCIES_LIST}")   

      create_common_source_group_structure("${COMMON_INCLUDE_DIRS}")

      include("${CMAKE_DIR}/core/dev-mode.cmake")

   else()
      make_header_only_project()

   endif()
   
endmacro()



macro(suppress_compiler_warnings)

   if (MSVC)
      if (PROJECT_IS_TOP_LEVEL AND TARGET spdlog)
         target_compile_options(spdlog PRIVATE /W0)
      endif()
      add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS)
      
   elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
      if (PROJECT_IS_TOP_LEVEL AND TARGET spdlog)
         target_compile_options(spdlog PRIVATE -w)
      endif()

   endif()

endmacro()

if (@SUPRESS_COMPILER_WARNING)
   suppress_compiler_warnings()
endif()



 