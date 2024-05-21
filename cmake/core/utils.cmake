## Defines Global Scoped CMake Variables and common reusable 
#  functions for various project domains.
#	
include(FetchContent)

macro(configure_boost_micro_unit_testing)
   #
   # See:  https://github.com/boost-ext/ut.git
   #
   if (CMAKE_CXX_STANDARD VERSION_GREATER_EQUAL 20)  
         set(BOOST_UT_ALLOW_CPM_USE OFF CACHE INTERNAL "" FORCE)
         #
         # At this time MSVC does not support C++ Modules in C++ 20+
         #
         if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            add_definitions(-DBOOST_UT_DISABLE_MODULE)
         else()
            add_definitions(-DENABLE_BOOST_UT_MODULE)
         endif()
      else()
         message("CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")
         message("\nWarning:")
         message(STATUS "The Boost micro/μt test framework requires C++ 20 and above.\n") 
      endif()
endmacro()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
   list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
   list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
endif()

if (EXISTS "${CMAKE_DIR}/core/network.cmake")
   include(${CMAKE_DIR}/core/network.cmake)
   check_for_internet_connection()
endif()

function(publish_common_include_dirs)
   message(STATUS "\nTarget Include Folders (Use: COMMON_INCLUDE_DIRS):")
   foreach(dir ${COMMON_INCLUDE_DIRS})
      message(STATUS "  ${dir}")
   endforeach()
endfunction()

function(publish_common_link_libs)
   message(STATUS "\nTarget Link Libraries (Use: COMMON_LINK_LIBRARIES):")
   foreach(lib ${COMMON_LINK_LIBRARIES})
      message(STATUS "  ${lib}")
   endforeach()
endfunction()

function(publish_optional_link_libs)
   message(STATUS "\nTarget Link Core Libraries (Use: OPTIONAL_LINK_LIBRARIES):")
   foreach(lib ${OPTIONAL_LINK_LIBRARIES})
      message(STATUS "  ${lib}")
   endforeach()
endfunction()

function(publish_pch_libraries)
   message(STATUS "\nPre-compiled header Libraries (Use: PCH_HEADERS):")
   foreach(lib ${PCH_HEADERS})
      message(STATUS "  ${lib}")
   endforeach()
endfunction()
##
# \brief Adds a directory to the 'COMMON_INCLUDE_DIRS' property.
#
# This function is designed to handle cases where the directory path contains spaces,
# which is necessary for certain pre-processors like CUDA. If the directory 
# path contains spaces, it is enclosed in quotes.
#
# \Example
#   A quoted list is necessary for CUDA pre-processor.
# 	Otherwise the following error:
#   	'nvcc fatal : A single input file is required for a non-link phase when an outputfile is specified'
#
# \param directory The directory to add to the 'COMMON_INCLUDE_DIRS' list.
#
# \code{.cmake}
# set_common_include_directory("/path/with spaces" "calling method name")
# \endcode
#
# \param directory The directory to add to the include directories.
#
# \see target_include_directories() and target_compile_options().
#
# https://cmake.org/cmake/help/latest/command/target_include_directories.html
#
function(set_common_include_directory directories called_from)

   foreach (dir ${directories})
      if (IS_DIRECTORY ${dir})
         file(TO_CMAKE_PATH "${dir}" dir)
         list(APPEND tmp_directories "${dir}")
      endif()
   endforeach()
   
   get_property(include_dirs DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
   list(APPEND tmp ${tmp_directories} ${include_dirs} ${COMMON_INCLUDE_DIRS})
   string(REPLACE ";;" ";" tmp "${tmp}")
   list(REMOVE_DUPLICATES tmp)
   foreach (dir ${tmp})
      if (IS_DIRECTORY ${dir})
         file(TO_CMAKE_PATH "${dir}" dir)
         list(APPEND updated_list "${dir}")
      endif()
   endforeach()
   
   set(COMMON_INCLUDE_DIRS "${updated_list}" CACHE INTERNAL "A list of target include directories." FORCE) 

endfunction()

function(set_precompiled_headers header_path)
   list(APPEND tmp "${header_path}")
   list(APPEND tmp "${PCH_HEADERS}")
   string(REPLACE ";;" ";" tmp "${tmp}")
   list(REMOVE_DUPLICATES tmp)
   set(PCH_HEADERS "${tmp}" CACHE INTERNAL "A list of headers to pre-compile." FORCE)
endfunction()

## \brief Use with 'target_link_directories'
#
# https://cmake.org/cmake/help/latest/command/target_link_directories.html
#
function(remove_duplicates input_list output_list)
   list(APPEND tmp "${input_list}")
   string(REPLACE ";;" ";" tmp "${tmp}")  # removes errors in older CMake versions.
   list(REMOVE_DUPLICATES tmp)
   set(output_list "${tmp}" PARENT_SCOPE)
endfunction()

## \brief Use with 'target_link_directories'
#
# https://cmake.org/cmake/help/latest/command/target_link_directories.html
#
function(set_common_link_directory directories)
   list(APPEND tmp "${directories}")
   list(APPEND tmp "${COMMON_LINK_DIRECTORIES}")
   string(REPLACE ";;" ";" tmp "${tmp}")  
   list(REMOVE_DUPLICATES tmp)
   set(COMMON_LINK_DIRECTORIES "${tmp}" CACHE INTERNAL "A list of target link directories." FORCE)
endfunction()

function(set_common_link_library libraries)
   list(APPEND tmp "${libraries}")
   list(APPEND tmp "${COMMON_LINK_LIBRARIES}")
   string(REPLACE "COMMON_LINK_LIBRARIES;" "" tmp "${tmp}") 
   string(REPLACE ";;" ";" tmp "${tmp}") 
   list(REMOVE_DUPLICATES tmp)
   set(COMMON_LINK_LIBRARIES "${tmp}" CACHE INTERNAL "A list of target link libraries." FORCE)
endfunction()

function(set_optional_link_library libraries)
   list(APPEND tmp "${libraries}")
   list(APPEND tmp "${OPTIONAL_LINK_LIBRARIES}")
   string(REPLACE "OPTIONAL_LINK_LIBRARIES;" "" tmp "${tmp}") 
   string(REPLACE ";;" ";" tmp "${tmp}") 
   list(REMOVE_DUPLICATES tmp)
   set(OPTIONAL_LINK_LIBRARIES "${tmp}" CACHE INTERNAL "A list of target link core libraries." FORCE)
endfunction()


## TODO: Make function/macro to acquire a drive letter
# if(WIN32)
#   set(DRIVE_LETTER "$ENV{USERPROFILE}")
#   string(SUBSTRING ${DRIVE_LETTER} 0 2 DRIVE_LETTER)
# else()
#   set(DRIVE_LETTER "$ENV{HOME}")
#   string(SUBSTRING ${DRIVE_LETTER} 0 1 DRIVE_LETTER)
# endif()

# \brief Function to fetch a package from a Git repository.
#
# This function clones or updates a Git repository to the specified target folder.
#
# \param target_folder The target folder where the package will be stored.
# \param url The URL of the Git repository.
# \param branch_or_tag The branch or tag to use when cloning or updating the repository.
#
function(fetch_package target_folder url branch_or_tag)
	
	set(deps_dir "${CPM_ROOT_DIR}")
	
	# Since 'deps_dir' is going be used as the working directory 
	# CMake requires that is exists. Otherwise the following clone
	# will fail and that without CMake letting you know why.
	#
	if (NOT EXISTS "${CPM_ROOT_DIR}")
		file(MAKE_DIRECTORY "${CPM_ROOT_DIR}")
	endif()
	
	set(git_event "pull")
	
	if (EXISTS "${CPM_ROOT_DIR}/${target_folder}")
	
		execute_process(
			COMMAND git pull
			WORKING_DIRECTORY "${CPM_ROOT_DIR}/${target_folder}"
			RESULT_VARIABLE exec_process_result
			OUTPUT_VARIABLE exec_process_output
		)
		
	else()
	
		set(git_event "clone")

		execute_process(
			COMMAND git clone --depth 1 --branch "${branch_or_tag}" --recursive "${url}" "${target_folder}"
			WORKING_DIRECTORY ${deps_dir}
			RESULT_VARIABLE exec_process_result
			OUTPUT_VARIABLE exec_process_output
		)
		
	endif()
	
	if (RESULT_VARIABLE)
		message("\nError:")
		message(STATUS "Unable to ${git_event} URL: ${url}!")
		message(STATUS "Reason: ${exec_process_output}\n")
	endif()
	
endfunction()

function(checkout_branch target_folder branch_or_tag)

	if (EXISTS "${target_folder}")
		execute_process(
			COMMAND git checkout "${branch_or_tag}"
			WORKING_DIRECTORY ${target_folder}
			RESULT_VARIABLE exec_process_result
			OUTPUT_VARIABLE exec_process_output)
	else()
		set(RESULT_VARIABLE 1)
		set(exec_process_output "git repository, '${target_folder}', does not exist!")
		
	endif()
		
	if (RESULT_VARIABLE)
		message("\nError:")
		message(STATUS "Unable to checkout branch or tag: ${branch_or_tag}!")
		message(STATUS "Repo: 'target_folder'")
		message(STATUS "Reason: ${exec_process_output}\n")
	endif()
	
endfunction()

macro(print_location)
    message("Current file: ${CMAKE_CURRENT_LIST_FILE}, Line: ${CMAKE_CURRENT_LIST_LINE}")
endmacro()
function(publish_dependencies_manifest)
   publish_common_include_dirs()
   publish_target_link_directories()
   publish_common_link_libs()
   publish_pch_libraries()
   message(STATUS "\n")
endfunction()

## Print dependency list for debugging.
#
# Example:
#  
#  set(dependencies)
#  list(APPEND dependencies "glaze, https://github.com/stephenberry/glaze.git, main;")
#  list(APPEND dependencies "argz, https://github.com/stephenberry/argz.git, main;")
#  list(APPEND dependencies "efftw, https://github.com/stephenberry/efftw.git, main;")
#  list(APPEND dependencies "ut, https://github.com/boost-ext/ut.git, master")
#
# Important Note when using CMake Lists:
#
# 	  Wrong: print_project_dependencies("Printing Dependencies:   dependencies)  # passes empty list
# 	  Wrong: print_project_dependencies("Printing Dependencies: ${dependencies}) # passes first item of list
#
# 	Correct: print_project_dependencies("Printing Dependencies:" "${dependencies}")
#
# @param caption The caption for the dependency list.
# @param items_list The list of dependencies in the format "name, url, tag".
##
function(print_project_dependencies caption items_list)
    message("${caption}")
    
    foreach(item ${items_list})
      
      # Removes white space.
      #
      string(REGEX REPLACE ",[ \t]*" ";" parsed_item "${item}")

      # Separate the parsed list into individual variables
      #
      separate_arguments(parsed_item)

      # Assign the values to separate variables
      list(GET parsed_item 0 name)
      list(GET parsed_item 1 url)
      list(GET parsed_item 2 tag)

      message("name:\t${name}")
      message("url:\t${url}")
      message("tag:\t${tag}\n")

    endforeach()

endfunction()

# This should go into a 'extend-behaviors.cmake'
#
function(check_if_precompiled_header name source_dir include_dir)

   #[[ Debugging Only
      message("name: ${name}")
      message("source_dir: ${source_dir}")
      message("include_dir: ${include_dir}")
   #]]

   #
   # Add any custom third-party dependency code you need here:
   #
   if (EXISTS "${source_dir}")

      set("${name}_FOUND" TRUE CACHE INTERNAL "${name}_FOUND is ${${name}_FOUND}" FORCE)

      # Boost Asio does not follow the type conventions for structuring
      # include paths. Therefore, the following code is must update
      # the include path for asio.
      #
      if ("asio" STREQUAL "${name}")
         set(include_dir "${CMAKE_BINARY_DIR}/_deps/${name}-src/asio/include")
         if (NOT EXISTS "${include_dir}")
            message("Required include directory, '${include_dir}', does not exist!")
            message(FATAL_ERROR)
         endif()
         set_common_include_directory("${include_dir}" "utils.cmake: ln:${CMAKE_CURRENT_LIST_LINE}")
         set_precompiled_headers("${include_dir}/asio.hpp")
         set(asio_SOURCE_DIR "${include_dir}" CACHE INTERNAL "" FORCE) # TODO: deprecated
         set(asio_INCLUDE_DIR "${include_dir}" CACHE INTERNAL "" FORCE)
      endif()
     
      if ("glaze" STREQUAL "${name}" AND EXISTS "${include_dir}") 
         if (NOT EXISTS "${include_dir}/glaze/glaze.hpp")
            message(FATAL_ERROR "Required header file, glaze.hpp, does not exist. Expected Location: '${include_dir}/glaze/glaze.hpp'")
         endif()
         set_precompiled_headers("${include_dir}/glaze/glaze.hpp")
         set(glaze_INCLUDE_DIR "${include_dir}" CACHE INTERNAL "" FORCE)
      endif()
      
      if ("efftw" STREQUAL "${name}" AND EXISTS "${include_dir}") 
         if (NOT EXISTS "${include_dir}/efftw/efftw.hpp")
            message(FATAL_ERROR "Required header file, efftw.hpp, does not exist. Expected Location: '${include_dir}/efftw/efftw.hpp'")
         endif()
         set_precompiled_headers("${include_dir}/efftw/efftw.hpp")
         set(efftw_INCLUDE_DIR "${include_dir}" CACHE INTERNAL "" FORCE)
      endif()
      
      if ("argz" STREQUAL "${name}" AND EXISTS "${include_dir}") 
         if (NOT EXISTS "${include_dir}/argz/argz.hpp")
            message(FATAL_ERROR "Required header file, argz.hpp, does not exist. Expected Location: '${include_dir}/argz/argz.hpp'")
         endif()
         set_precompiled_headers("${include_dir}/argz/argz.hpp")
         set(argz_INCLUDE_DIR "${include_dir}" CACHE INTERNAL "" FORCE)
      endif()

   endif()

   if (NOT "beve" STREQUAL "${name}" AND NOT "json-schema-generator" STREQUAL "${name}" AND NOT "matplotplusplus" STREQUAL "${name}")
      #
      # Don't show the following for beve:
      #
      if (NOT EXISTS ${include_dir} AND NETWORK_CHECK_RESULT_STATUS)
         message(STATUS "${LINE_DIV}")
         message("Warning:")
         message(STATUS " The include path for the ${name} repository "
         "requires a custom implementation.\n See: utils.cmake "
         "to configure a custom include path for this repository")
         message(STATUS "${LINE_DIV}\n")
      endif()
   endif()  
    
endfunction()

# fetch_content_and_make_available from a repositories.
#
# This function uses 'FetchContent_Declare' and 'FetchContent_MakeAvailable'.
#
# Example Use:
#
#   list(APPEND dependencies "glaze,   https://github.com/stephenberry/glaze.git, main")
#   list(APPEND dependencies "argz,    https://github.com/stephenberry/argz.git,  main")
#   list(APPEND dependencies "efftw,   https://github.com/stephenberry/efftw.git, main")
#   list(APPEND dependencies "ut,      https://github.com/boost-ext/ut.git,       master")
#   list(APPEND dependencies "Eigen3,  https://gitlab.com/libeigen/eigen.git,     3.4.0")
#
#   GOOD: fetch_content_and_make_available("${dependencies}")
#
#   WRONG: fetch_content_and_make_available(${dependencies})
#   WRONG: fetch_content_and_make_available(dependencies)
#
# @param items_list The list of dependencies in the format "name, url, tag".
# 'tag' may be: commit_hash, tag, or branch name
#
function(fetch_content_and_make_available items_list)

   list(APPEND deps_list "${items_list}") 
   list(REMOVE_DUPLICATES deps_list)
   message(STATUS "")
   
   foreach(item ${deps_list})
        
      string(REGEX REPLACE ",[ \t]*" ";" parsed_item "${item}")

      # Now separate the args of each list item into its parts.
      #
      separate_arguments(parsed_item)

      # Assign the values to separate variables
      #
      list(GET parsed_item 0 name)
      list(GET parsed_item 1 url)
      list(GET parsed_item 2 tag)

      set(source_dir "${CMAKE_BINARY_DIR}/_deps/${name}-src")
      set(include_dir "${CMAKE_BINARY_DIR}/_deps/${name}-src/include")
      
      if (NOT DEFINED @ALWAYS_FETCH_DEPENDENCIES)
         option(@ALWAYS_FETCH_DEPENDENCIES "Always Fetch Dependencies." ON)
      endif()
      
      if (@ALWAYS_FETCH_DEPENDENCIES AND NETWORK_CHECK_RESULT_STATUS)
         #
         # We ARE fetching dependencies and therefore this should be 'OFF':
         #
         set(FETCHCONTENT_FULLY_DISCONNECTED OFF)
        
      else()
      
         if (EXISTS "${source_dir}")
            message(STATUS "  Found Package ${name}.")
            set(FETCHCONTENT_FULLY_DISCONNECTED ON)
            
         else()
            # 
            # We have to fetch...the repo is not available.
            #
            set(FETCHCONTENT_FULLY_DISCONNECTED OFF)
         endif()
      
      endif()
      
      if (NOT EXISTS "${source_dir}")
         message(STATUS "Fetching '${name}':")
         message(STATUS " url:\t${url}")
         message(STATUS " tag:\t${tag}")
      # else()
      #   message(STATUS "Updating ${name}...")
      endif()
   
      # Basic FetchContent_Declare Args (the following is not a complete list):
      # 	name: 
      #     This is the name by which the external content will be referred to 
      #     in your project.
      # 	
      #   URL: 
      #     This is the URL where the external content can be found. It could
      #	  be a Git repository, an HTTP URL, a local directory, etc.
      #
      # 	GIT_REPOSITORY, GIT_TAG: 
      #     If the content is a Git repository, these arguments specify the 
      #     repository and branch or tag to use.
      #
      # 	SOURCE_DIR: 
      #      This is an optional argument that allows you to specify a directory
      #      where the content should be downloaded or cloned.
      #
      # 	BINARY_DIR: 
      #      This is an optional argument that allows you to specify where the 
      #      content should be built.
      #
      # 	CONFIGURE_COMMAND, BUILD_COMMAND, INSTALL_COMMAND, TEST_COMMAND, GIT_PROGRESS, etc.: 
      #      These are optional commands that specify how the content should 
      #      be configured, built, installed, and tested.
      #
      set(FETCHCONTENT_QUIET TRUE)
      
      FetchContent_Declare(
         "${name}"
         GIT_REPOSITORY "${url}"
         GIT_TAG "${tag}" # may branch, tag, or commit hash
         GIT_SHALLOW TRUE
         GIT_SUBMODULES_RECURSE TRUE
         # GIT_PROGRESS FALSE
      )
      FetchContent_MakeAvailable(${name})
      
      # Headers to be marked for pre-compilation:
      #
      check_if_precompiled_header("${name}" "${source_dir}" "${include_dir}")
   
      if (EXISTS "${include_dir}")
         set(${name}_FOUND TRUE CACHE INTERNAL "" FORCE)
         set_common_include_directory("${include_dir}" "called from utils.cmake: ln:${CMAKE_CURRENT_LIST_LINE}")
      endif()
      
   endforeach()
  
endfunction()

# TODO:
# url, description and so forth should be configurable.
# Add a function to set these properties separately.
#
# Examples:
# set_project_info("clutter_patch" 2 1 0)    -> v2.1.0
# set_project_info("clutter_patch" "" "" "") -> vYY.MM.DD (e.g., v24.01.02)
# set_project_info("clutter_patch" "-latest" "" "") , etc.
#
# This function seaches for "${CMAKE_CURRENT_LIST_DIR}/include/clutter_patch/version.h.in" and 
# generates the output file "${CMAKE_CURRENT_LIST_DIR}/include/clutter_patch/version.hpp".
function(set_project_info project_name project_url project_description major_version minor_version patch_version)

   string(TIMESTAMP CURRENT_DATE "%Y-%m-%d")

   string(SUBSTRING ${CURRENT_DATE} 2 2 MY_PROJECT_VERSION_MAJOR) # YY
   string(SUBSTRING ${CURRENT_DATE} 5 2 MY_PROJECT_VERSION_MINOR) # MM
   string(SUBSTRING ${CURRENT_DATE} 8 2 MY_PROJECT_VERSION_PATCH) # DD

   if (NOT major_version STREQUAL "")
     set(MY_PROJECT_VERSION_MAJOR ${major_version})
   endif()

   if (NOT minor_version STREQUAL "")
     set(MY_PROJECT_VERSION_MINOR ${minor_version})
   endif()

   if (NOT patch_version STREQUAL "")
     set(MY_PROJECT_VERSION_PATCH ${patch_version})
   endif()

   set(MY_PROJECT_VERSION "v${MY_PROJECT_VERSION_MAJOR}.${MY_PROJECT_VERSION_MINOR}.${MY_PROJECT_VERSION_PATCH}")
   set(MY_PROJECT_NAME "${project_name}")
   set(MY_PROJECT_DESCRIPTION  "${project_description}")
   set(MY_PROJECT_HOMEPAGE_URL "${project_url}")
   set(MY_PROJECT_COMPANY_URL "https://github.com/openalgz" CACHE INTERNAL "" FORCE)
   set(MY_COPYRIGHT "(c) Open Source Team Tools 2024" CACHE INTERNAL "" FORCE)
   
   set(MY_PROJECT_VERSION ${MY_PROJECT_VERSION} CACHE INTERNAL "" FORCE)

   if (EXISTS "${CURRENT_PROJECT_DIR}/include/${MY_PROJECT_NAME}/version.h.in")
      configure_file("${CURRENT_PROJECT_DIR}/include/${MY_PROJECT_NAME}/version.h.in" "${CURRENT_PROJECT_DIR}/include/${MY_PROJECT_NAME}/version.hpp" @ONLY)      
   else()
      message(FATAL_ERROR "Missing ${MY_PROJECT_NAME} Version Input: ${CURRENT_PROJECT_DIR}/include/${MY_PROJECT_NAME}/version.h.in")
   endif()
   
   set(GIT_VERSION_TAG ${GIT_VERSION_TAG} CACHE INTERNAL "" FORCE)

endfunction()

# Tries to get a tag or branch name.
#
function(set_project_info_by_git project_name project_url project_description template_path)

   string(TIMESTAMP CURRENT_DATE "%Y-%m-%d")
   string(SUBSTRING ${CURRENT_DATE} 2 2 YY)
   string(SUBSTRING ${CURRENT_DATE} 5 2 MM)
   string(SUBSTRING ${CURRENT_DATE} 8 2 DD)

   # Retrieve Git branch name
   execute_process(
       COMMAND git rev-parse --abbrev-ref HEAD
       WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
       OUTPUT_VARIABLE GIT_VERSION_TAG
       ERROR_VARIABLE git_branch_error
       OUTPUT_STRIP_TRAILING_WHITESPACE
       RESULT_VARIABLE git_branch_result
   )

   if(SHOW_DIAGNOSTICS AND git_branch_result EQUAL 0)
       message("Git Branch Name: ${GIT_VERSION_TAG}")
   endif()

   # Retrieve latest Git tag
   execute_process(
       COMMAND git describe --tags --abbrev=0
       WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
       OUTPUT_VARIABLE GIT_VERSION_TAG
       ERROR_VARIABLE git_tag_error
       OUTPUT_STRIP_TRAILING_WHITESPACE
       RESULT_VARIABLE git_tag_result
   )

   if(SHOW_DIAGNOSTICS AND git_tag_result EQUAL 0)
       message("Latest Git Tag: ${GIT_VERSION_TAG}")
   endif()

   if ((NOT git_branch_result EQUAL 0 AND NOT git_tag_result EQUAL 0) OR GIT_VERSION_TAG STREQUAL "")
      set(GIT_VERSION_TAG "v${YY}.${MM}.${DD}")
   endif()
  
   set(MY_PROJECT_VERSION "${GIT_VERSION_TAG}" CACHE INTERNAL "" FORCE)
   string(REGEX MATCH "v([0-9]+)\\.([0-9]+)\\.([0-9]+)" version_match "${GIT_VERSION_TAG}")
   
   if(version_match)
       set(MY_PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1} CACHE INTERNAL "" FORCE)
       set(MY_PROJECT_VERSION_MINOR ${CMAKE_MATCH_2} CACHE INTERNAL "" FORCE)
       set(MY_PROJECT_VERSION_PATCH ${CMAKE_MATCH_3} CACHE INTERNAL "" FORCE)
   endif()
   
   set(MY_PROJECT_VERSION ${MY_PROJECT_VERSION} CACHE INTERNAL "" FORCE)
   set(MY_PROJECT_NAME "${project_name}" CACHE INTERNAL "" FORCE)
   set(MY_PROJECT_DESCRIPTION  "${project_description}" CACHE INTERNAL "" FORCE)
   set(MY_PROJECT_HOMEPAGE_URL "${project_url}" CACHE INTERNAL "" FORCE)
   set(MY_PROJECT_COMPANY_URL "https://github.com/openalgz" CACHE INTERNAL "" FORCE)
   set(MY_COPYRIGHT "(c) Open Source Team Tools 2024" CACHE INTERNAL "" FORCE)
   
   # The template file name in the template path is expected to be of the form: 'version.h.in'
   #
   # How this works:
   # If template_path is /path/to/version.h.in, then output_path will be /path/to/version.hpp.
   #
   string(REPLACE ".in" "pp" output_path "${template_path}")

   if (EXISTS "${template_path}")
     configure_file("${template_path}" "${output_path}" @ONLY)
   else()
     message(FATAL_ERROR "Missing Input Template. Expected Path: '${template_path}'")
   endif()  
   
   set(GIT_VERSION_TAG ${GIT_VERSION_TAG} CACHE INTERNAL "" FORCE)
   
endfunction()

function(force_project_version version project_name url description template_path)

   set(GIT_VERSION_TAG "v${version}")
   set(MY_PROJECT_VERSION "${GIT_VERSION_TAG}" CACHE INTERNAL "" FORCE)
   string(REGEX MATCH "v([0-9]+)\\.([0-9]+)\\.([0-9]+)" version_match "${GIT_VERSION_TAG}")
   
   if(version_match)
       set(MY_PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1} CACHE INTERNAL "" FORCE)
       set(MY_PROJECT_VERSION_MINOR ${CMAKE_MATCH_2} CACHE INTERNAL "" FORCE)
       set(MY_PROJECT_VERSION_PATCH ${CMAKE_MATCH_3} CACHE INTERNAL "" FORCE)
   endif()
   
   set(MY_PROJECT_VERSION ${MY_PROJECT_VERSION} CACHE INTERNAL "" FORCE)

   set(MY_PROJECT_NAME "${project_name}" CACHE INTERNAL "" FORCE)
   set(MY_PROJECT_DESCRIPTION  "${description}" CACHE INTERNAL "" FORCE)
   set(MY_PROJECT_HOMEPAGE_URL "${url}" CACHE INTERNAL "" FORCE)
   set(MY_PROJECT_COMPANY_URL "https://github.com/openalgz" CACHE INTERNAL "" FORCE)
   set(MY_COPYRIGHT "(c) Open Source Team Tools 2024" CACHE INTERNAL "" FORCE)
   
   # The template file name in the template path is expected to be of the form: 'version.h.in'
   #
   # How this works:
   # If template_path is /path/to/version.h.in, then output_path will be /path/to/version.hpp.
   #
   string(REPLACE ".in" "pp" output_path "${template_path}")

   if (EXISTS "${template_path}")
     configure_file("${template_path}" "${output_path}" @ONLY)
   else()
     message(FATAL_ERROR "Missing Input Template. Expected Path: '${template_path}'")
   endif()  
   
   set(GIT_VERSION_TAG ${GIT_VERSION_TAG} CACHE INTERNAL "" FORCE)
   
endfunction()

function(show_project_manifest)
   message(STATUS "\n${LINE_DIV}")
   message( "Project Manifest:")
   message(STATUS "  Project Name: ${MY_PROJECT_NAME}")
   message(STATUS "  Project Version: ${MY_PROJECT_VERSION}")
   message(STATUS "  Project Description: ${MY_PROJECT_DESCRIPTION}")
   message(STATUS "  Project Homepage URL: ${MY_PROJECT_HOMEPAGE_URL}")
   message(STATUS "  Project Company URL: ${MY_PROJECT_COMPANY_URL}")
   message(STATUS "  Copyright: ${MY_COPYRIGHT}")
   message(STATUS "${LINE_DIV}\n")
endfunction()

macro(make_header_only_project)

   add_library(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE)
   add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME}_${PROJECT_NAME})

   if (MSVC)
      string(REGEX MATCH "\/cl(.exe)?$" matched_cl ${CMAKE_CXX_COMPILER})
      if (matched_cl)
         # for a C++ standards compliant preprocessor, not needed for clang-cl
         target_compile_options(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE "/Zc:preprocessor" /GL /permissive- /Zc:lambda)
         target_link_options(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE /LTCG /INCREMENTAL:NO)
      endif()
   else()
      target_compile_options(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE "-Wno-missing-braces")
   endif()

   set_property(TARGET ${PROJECT_NAME}_${PROJECT_NAME} PROPERTY EXPORT_NAME ${PROJECT_NAME})

   target_compile_features(${PROJECT_NAME}_${PROJECT_NAME} INTERFACE cxx_std_20)
   target_include_directories(
      ${PROJECT_NAME}_${PROJECT_NAME} ${warning_guard}
       INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
   )

   if(NOT CMAKE_SKIP_INSTALL_RULES)
     include(${CMAKE_DIR}/core/install-rules.cmake)
   endif()
   
endmacro()

macro(make_project_executable project_name sources) 
   if(NOT TARGET ${project_name})
      message(STATUS "Adding Project: ${project_name}")
      add_executable(${project_name} ${sources} ${source_group_includes})  
      target_include_directories(${project_name} PUBLIC ${COMMON_INCLUDE_DIRS})
      target_link_libraries(${project_name} ${COMMON_LINK_LIBRARIES} ${TARGET_OPTIONAL_LIBRARIES})
      target_precompile_headers(${project_name} PRIVATE ${PCH_HEADERS})
   endif()
endmacro()


