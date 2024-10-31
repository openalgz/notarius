#[[
   Add cpp module dependencies here:
#]]
list(APPEND CPP_MODULE_DEPENDENCIES "notarius, https://github.com/openalgz/notarius.git, v0.1.6")

#[[
   Add other external dependencies here
#]]
list(APPEND EXTERNAL_DEPENDENCIES "glaze, https://github.com/stephenberry/glaze.git, v3.6.2")
list(APPEND EXTERNAL_DEPENDENCIES "ut,    https://github.com/openalgz/ut.git,        v0.0.3")

if (CMAKE_VERBOSE_MAKEFILE)
   get_filename_component(CURRENT_MODULE_NAME "${CMAKE_CURRENT_LIST_FILE}" NAME)
   message(STATUS "- importing ${CURRENT_MODULE_NAME}.")
endif()

include("${CMAKE_DIR}/core/FetchContentEx.cmake")

function(fetch_and_configure_dependencies)
    make_grouped_cpp_modules_library("notarius" "${CPP_MODULE_DEPENDENCIES}")
    fetch_content_and_make_available("${EXTERNAL_DEPENDENCIES}")
endfunction()
