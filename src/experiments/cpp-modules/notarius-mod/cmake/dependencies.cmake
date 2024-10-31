#[[
   Add cpp module dependencies here:
#]]
list(APPEND CPP_MODULE_DEPENDENCIES "notarius, https://github.com/openalgz/notarius.git, main")

#[[
   Add other external library dependencies here:
#]]
list(APPEND EXTERNAL_DEPENDENCIES "glaze, https://github.com/stephenberry/glaze.git, v3.6.2")
list(APPEND EXTERNAL_DEPENDENCIES "ut,    https://github.com/openalgz/ut.git,        v0.0.3")

if (CMAKE_VERBOSE_MAKEFILE)
   get_filename_component(CURRENT_MODULE_NAME "${CMAKE_CURRENT_LIST_FILE}" NAME)
   message(STATUS "- importing ${CURRENT_MODULE_NAME}.")
endif()

include("${CMAKE_DIR}/core/FetchContentEx.cmake")

function(fetch_and_configure_dependencies fetched_include_directories)
    make_cpp_modules_library("This argument ignored when 'grouped' is FALSE" "${CPP_MODULE_DEPENDENCIES}" FALSE)
    fetch_content_and_make_available("${EXTERNAL_DEPENDENCIES}" fetched_include_directories)
    set(fetched_include_directories ${fetched_include_directories} PARENT_SCOPE)   
endfunction()
