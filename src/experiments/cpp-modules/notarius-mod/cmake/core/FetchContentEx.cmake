if (CMAKE_VERBOSE_MAKEFILE)
    get_filename_component(CURRENT_MODULE_NAME "${CMAKE_CURRENT_LIST_FILE}" NAME)
    message(STATUS "- importing ${CURRENT_MODULE_NAME}.")
endif()

function(create_module_library target_name generated_cpp_module_path fetched_include_directories)
    # Validate input parameters
    if (NOT target_name OR NOT generated_cpp_module_path OR NOT fetched_include_directories)
        message(FATAL_ERROR "Missing required parameters for create_module_library function.")
        return()
    endif()

    # Create a static library for the module
    add_library("${target_name}" STATIC)
    target_sources("${target_name}" PRIVATE "${generated_cpp_module_path}")
    target_compile_features("${target_name}" PUBLIC cxx_std_20)

    # Set include directories for dependencies
    target_include_directories("${target_name}" PUBLIC "${CMAKE_BINARY_DIR}/modules")
    target_include_directories("${target_name}" PUBLIC ${fetched_include_directories})

    # Add module-specific compiler flags
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options("${target_name}" PRIVATE -fmodules-ts)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        target_compile_options("${target_name}" PRIVATE /std:c++20)
    else()
        message(FATAL_ERROR "Unsupported compiler for C++20 modules: ${CMAKE_CXX_COMPILER_ID}")
    endif()

    # Add preprocessor definitions for the module
    target_compile_definitions("${target_name}" PRIVATE "${target_name}_MODULE")
endfunction()

function(find_string_in_file file_path search_string file_found substr_found)
    if (NOT EXISTS "${file_path}")
        set(file_found FALSE PARENT_SCOPE)
        set(substr_found FALSE PARENT_SCOPE)
        return()
    else()
        set(file_found TRUE PARENT_SCOPE)
    endif()

    file(READ "${file_path}" file_content)
    string(FIND "${file_content}" "${search_string}" found_index)

    if (found_index GREATER -1)
        set(substr_found TRUE PARENT_SCOPE)
    else()
        set(substr_found FALSE PARENT_SCOPE)
    endif()
endfunction()

include(FetchContent)

option(@ALWAYS_FETCH_DEPENDENCIES "Always fetch dependencies even if they exist locally." ON)

function(fetch_content_and_make_available deps_list)
    list(REMOVE_DUPLICATES deps_list)
    set(FETCHED_INCLUDE_DIRECTORIES "")
  
    foreach(item ${deps_list})
        string(REGEX REPLACE ",[ \t]*" ";" parsed_item "${item}")
        separate_arguments(parsed_item)
        list(GET parsed_item 0 name)
        list(GET parsed_item 1 url)
        list(LENGTH parsed_item len)

        if (len GREATER 2)
            list(GET parsed_item 2 tag)
        else()
            set(tag "main")
        endif()

        set(source_dir "${CMAKE_BINARY_DIR}/_deps/${name}-src")
        set(include_dir "${source_dir}/include")

        if (DEFINED @ALWAYS_FETCH_DEPENDENCIES OR NOT EXISTS "${source_dir}")
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
        elseif(EXISTS "${source_dir}")
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

    list(REMOVE_DUPLICATES FETCHED_INCLUDE_DIRECTORIES)
    set(FETCHED_INCLUDE_DIRECTORIES "${FETCHED_INCLUDE_DIRECTORIES}" PARENT_SCOPE)
    
    if (CMAKE_VERBOSE_MAKEFILE)
        message(STATUS "Dependency Include Directories Manifest:")
        foreach(item ${FETCHED_INCLUDE_DIRECTORIES})
            message(STATUS " - '${item}'")
        endforeach()
    endif()
endfunction()

function(check_if_precompiled_header name source_dir include_dir substr_found)
    set(${substr_found} FALSE PARENT_SCOPE)

    if (EXISTS "${source_dir}/CMakeLists.txt")
        file(READ "${source_dir}/CMakeLists.txt" cmake_contents)
        if (cmake_contents MATCHES "target_precompile_headers")
            message(STATUS "${name} uses precompiled headers")
            set(${substr_found} TRUE PARENT_SCOPE)
        endif()
    endif()
endfunction()

function(write_module_file generated_cpp_module_path target_name)
    string(TIMESTAMP TIME_STAMP "%Y-%m-%d %H:%M:%S" UTC)
    file(WRITE "${generated_cpp_module_path}" "// DO NOT EDIT - CMake Generated\n// File: ${target_name}.cppm\n// Last Updated: ${TIME_STAMP}\n\n")
endfunction()

function(make_cpp_modules_library target_name deps_list grouped)
    # Fetch and prepare dependencies
    fetch_content_and_make_available("${deps_list}")

    # Where the CPP Modules are being placed:
    set(MODULES_DIR "${CMAKE_BINARY_DIR}/modules")
    file(MAKE_DIRECTORY "${MODULES_DIR}")
    
    if (grouped)
        set(generated_cpp_module_path "${MODULES_DIR}/${target_name}.cppm")
        write_module_file("${generated_cpp_module_path}" "${target_name}")
    endif()

    # Append include directives for each dependency and check for precompiled headers
    foreach(dep ${deps_list})
        string(REGEX REPLACE ",[ \t]*" ";" dep "${dep}")
        separate_arguments(dep)
        list(GET dep 0 dep_name)

        # Define the include and source paths for the dependency
        set(dep_source "${CMAKE_BINARY_DIR}/_deps/${dep_name}-src")
        set(dep_include "${dep_source}/include")
        
        # By Convention the Pre-processor defined is the name of the library (capitalized) plus '_MODULE'
        string(TOUPPER "${dep_name}_MODULE" CPP_MODULE_NAME)
        add_definitions("-D${CPP_MODULE_NAME}")

        set(module_include_path ${dep_include}/${dep_name}/${dep_name}.hpp)

        if (NOT EXISTS "${module_include_path}")
            message(FATAL_ERROR "What: ${module_include_path}")
        endif()
        
        # Check for module definition in the header file
        find_string_in_file("${module_include_path}" "${CPP_MODULE_NAME}" file_found substr_found)
        
        if (NOT file_found)
            message(FATAL_ERROR "Error: File Path '${module_include_path}' was not found. Unable to complete module generation for '${dep_name}'")
        else()
            if (NOT grouped)
                set(generated_cpp_module_path "${MODULES_DIR}/${dep_name}.cppm")
                write_module_file("${generated_cpp_module_path}" "${dep_name}")
            endif()

            file(APPEND "${generated_cpp_module_path}" "#include <${dep_name}/${dep_name}.hpp>\n")
        endif()

        if (NOT substr_found)
            message(WARNING "Header file '${module_include_path}' does not contain expected module preprocessor definition '${CPP_MODULE_NAME}'!")
        endif()

        if (NOT grouped)
            create_module_library("${dep_name}" "${generated_cpp_module_path}" "${FETCHED_INCLUDE_DIRECTORIES}")
        endif()
    endforeach()

    if (grouped)
        create_module_library("${target_name}" "${generated_cpp_module_path}" "${FETCHED_INCLUDE_DIRECTORIES}")
    endif()
endfunction()

function(make_executable src_file libs)
    get_filename_component(test_name ${src_file} NAME_WE)
    add_executable(${test_name} ${src_file})
    target_link_libraries(${test_name} PRIVATE ${libs})
    message(STATUS "Added executable: ${test_name} from source: ${src_file}")
endfunction()
