if (CMAKE_VERBOSE_MAKEFILE)
   get_filename_component(CURRENT_MODULE_NAME "${CMAKE_CURRENT_LIST_FILE}" NAME)
   message(STATUS "- importing ${CURRENT_MODULE_NAME}.")
endif()

#[[
# Example:

   find_string_in_file("${file_path}" "${search_string}" found)

   if(found)
       message(STATUS "${search_string} was found in ${file_path}")
   else()
       message(STATUS "${search_string} was NOT found in ${file_path}")
   endif()
#]]
function(find_string_in_file file_path search_string result_var)
    # Read the file content
    file(READ "${file_path}" file_content)

    # Search for the string in the file content
    string(FIND "${file_content}" "${search_string}" found_index)

    # Set the result variable to true if found, otherwise false
    if(found_index GREATER -1)
        set(${result_var} TRUE PARENT_SCOPE)
    else()
        set(${result_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

include(FetchContent)

function(fetch_content_and_make_available deps_list)
    list(REMOVE_DUPLICATES deps_list)
    set(FETCHED_INCLUDE_DIRECTORIES "")
  
    foreach(item ${deps_list})
        string(REGEX REPLACE ",[ \t]*" ";" parsed_item "${item}")
        separate_arguments(parsed_item)
        list(GET parsed_item 0 name)
        list(GET parsed_item 1 url)
        list(LENGTH parsed_item len)

        if (${len} GREATER 2)
            list(GET parsed_item 2 tag)
        else()
            set(tag "main")
        endif()

        set(source_dir "${CMAKE_BINARY_DIR}/_deps/${name}-src")
        set(include_dir "${source_dir}/include")

        if (@ALWAYS_FETCH_DEPENDENCIES OR NOT EXISTS "${source_dir}")
            set(FETCHCONTENT_FULLY_DISCONNECTED OFF)
            message(STATUS "Fetching '${name}': URL: ${url}, Tag: ${tag}")
            FetchContent_Declare(
                "${name}"
                GIT_REPOSITORY "${url}"
                GIT_TAG "${tag}"
                GIT_SHALLOW TRUE
                GIT_SUBMODULES_RECURSE TRUE
            )
            FetchContent_MakeAvailable(${name})
        else()
            set(FETCHCONTENT_FULLY_DISCONNECTED ON)
            message(STATUS "Found Package ${name}.")
        endif()

        if (NOT EXISTS "${source_dir}")
            message(FATAL_ERROR "Failed to fetch ${name} from ${url}")
        endif()

        if (EXISTS "${include_dir}")
            list(APPEND FETCHED_INCLUDE_DIRECTORIES "${include_dir}")
        else()
            list(APPEND FETCHED_INCLUDE_DIRECTORIES "${source_dir}")
        endif()
    endforeach()

    set(FETCHED_INCLUDE_DIRECTORIES "${FETCHED_INCLUDE_DIRECTORIES}" PARENT_SCOPE)
    list(REMOVE_DUPLICATES FETCHED_INCLUDE_DIRECTORIES)
    
    if (CMAKE_VERBOSE_MAKEFILE)
      message(STATUS "Dependencie Include Directories Manifest:")
      foreach(item ${FETCHED_INCLUDE_DIRECTORIES})
         message(STATUS " - '${item}'")
      endforeach()
    endif()
    
endfunction()

function(check_if_precompiled_header name source_dir include_dir result_var)
    set(${result_var} FALSE PARENT_SCOPE)

    if(EXISTS "${source_dir}/CMakeLists.txt")
        file(READ "${source_dir}/CMakeLists.txt" cmake_contents)
        if(cmake_contents MATCHES "target_precompile_headers")
            message(STATUS "${name} uses precompiled headers")
            set(${result_var} TRUE PARENT_SCOPE)
        endif()
    endif()
endfunction()

#[[ Description:

  If the header only library is named 'notarius' the 
  convention used here assumes NOTARIUS_MODULE.
  
  For Example, 'notarius.hpp' would be expected to contain something like:
  
   #ifdef NOTARIUS_MODULE
      module notarius;
   #endif
   
   namespace nsp
   {
      .
      .
      .
   }
   
   #ifdef NOTARIUS_MODULE
      export
      {
         namespace nsp
         {
            // The things to export . . .
            //
            using nsp::log_level;
            using nsp::notarius_opts_t;
            using nsp::notarius_t;
         }
      }
   #endif
#]]
function(make_grouped_cpp_modules_library target_name deps_list)

    # Fetch and prepare dependencies
    #
    fetch_content_and_make_available("${deps_list}")

    # Where the CPP Modules are being placed:
    #
    set(MODULES_DIR "${CMAKE_BINARY_DIR}/modules")
    file(MAKE_DIRECTORY "${MODULES_DIR}")
    set(generated_cpp_module_path "${MODULES_DIR}/${target_name}.cppm")
    string(TIMESTAMP GENERATED_TIME "%Y-%m-%d %H:%M:%S" UTC)
    file(WRITE "${generated_cpp_module_path}" "// DO NOT EDIT - CMake Generated\n// File: ${target_name}.cppm\n// Last Updated: ${GENERATED_TIME}\n\n")

    # Append include directives for each dependency and check for precompiled headers
    #
    foreach(dep ${deps_list})
        string(REGEX REPLACE ",[ \t]*" ";" dep "${item}")
        separate_arguments(dep)
        list(GET parsed_dep 0 dep_name)
        
        # Define the include and source paths for the dependency
        #
        set(dep_source "${CMAKE_BINARY_DIR}/_deps/${dep_name}-src")
        set(dep_include "${dep_source}/include")
        
        string(TOUPPER "${dep_name}_MODULE" CPP_MODULE_NAME)
        add_definitions("-D${CPP_MODULE_NAME}")
        
        # Check for module definition in the header file:
        #
        find_string_in_file("${dep_include}/${dep_name}.hpp" "${CPP_MODULE_NAME}" found)
        
      if (NOT found)
         message("\nWarning:\n"
                 "  Could not find preprocessor definition '${CPP_MODULE_NAME}' in '${${dep_include}/${dep_name}.hpp}'\n"
                 "  Therefore module exports may not be available.\n\n"
         )
      endif()
    
      file(APPEND "${generated_cpp_module_path}" "#include <${dep_name}/${dep_name}.hpp")
    endforeach()
    
    # Create a static library for the module
    #
    add_library("${target_name}" STATIC)
    target_sources("${target_name}" PRIVATE "${generated_cpp_module_path}")
    target_compile_features("${target_name}" PUBLIC cxx_std_20)

    # Set include directories for dependencies
    target_include_directories("${target_name}" PUBLIC "${CMAKE_BINARY_DIR}/modules")
    target_include_directories("${target_name}" PUBLIC ${FETCHED_INCLUDE_DIRECTORIES})

    # Add module-specific compiler flags
    #
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options("${target_name}" PRIVATE -fmodules-ts)
        
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        target_compile_options("${target_name}" PRIVATE /std:c++20)
        
    else()
        message(FATAL_ERROR "Unsupported compiler for C++20 modules: ${CMAKE_CXX_COMPILER_ID}")
        
    endif()
endfunction()

function(make_executable src_file libs)
    get_filename_component(test_name ${src_file} NAME_WE)
    add_executable(${test_name} ${src_file})
    target_link_libraries(${test_name} PRIVATE ${libs})
    message(STATUS "Added executable: ${test_name} from source: ${src_file}")
endfunction()
