#if (DEFINED SHOW_MODULE_IMPORTS)
#	get_filename_component(CURRENT_MODULE_NAME "${CMAKE_CURRENT_LIST_FILE}" NAME)
#	message(STATUS "  Importing ${CURRENT_MODULE_NAME}.")
#endif()

##
# \brief Check for internet connection by pinging Google's primary DNS server.
#
# This function checks for a internet connection by pinging the specified URL/IP.
#
# The internet connection status is stored in the  CACHED INTERNAL
# `NETWORK_CHECK_RESULT_STATUS` variable.
#
# If a Internet is not found then  FETCHCONTENT_FULLY_DISCONNECTED is turned ON 
# otherwise OFF.
#
# \Example
#   check_for_internet_connection()
#
#   if(NOT NETWORK_CHECK_RESULT_STATUS)
#      ...
#   else()
#      ...
#   endif()
#
function (check_for_internet_connection)

   # Using Google's primary Public DNS server. It would be highly unusual for this 
   # to be down.
   #
   set(url_target "8.8.8.8")

   # We only inform the user once that we are checking, but we still
   # check for a connection whenever this function is called.
   #
   if (NOT NETWORK_CHECK_RESULT_STATUS)
      message(STATUS "Checking for Internet Connection '${url_target}' . . . waiting")
   endif()

   if(WIN32)
     execute_process(
         COMMAND ping "${url_target}" -n 1
         ERROR_QUIET OUTPUT_QUIET
         RESULT_VARIABLE CONNECTED
     )
   elseif(APPLE)
     execute_process(
         COMMAND ping -c 1 "${url_target}"
         ERROR_QUIET OUTPUT_QUIET
         RESULT_VARIABLE CONNECTED
     )
   else(LINUX)
     execute_process(
         COMMAND ping "${url_target}" -c 1 
         ERROR_QUIET OUTPUT_QUIET
         RESULT_VARIABLE CONNECTED
     )
   endif()

   if (DEBUG_MESSAGES)
     message(STATUS "Debug: Internet Connection Check: ${RESULT_VARIABLE} ${CONNECTED}")
   endif()
    
   # When FETCHCONTENT_FULLY_DISCONNECTED is set to TRUE or ON, it prevents CMake
   # FetchContent_Declare from attempting to update or retrieve the content specified 
   # during project generation. Instead, CMake will rely on the existing content
   # present in the source tree.

   # Reset FETCHCONTENT_FULLY_DISCONNECTED Status
   # We only want it on if there is no internet connection.
   #
   set(FETCHCONTENT_FULLY_DISCONNECTED OFF)

   if(NOT CONNECTED EQUAL 0)
   
      set(FETCHCONTENT_FULLY_DISCONNECTED ON)
      set(NETWORK_CHECK_RESULT_STATUS FALSE CACHE INTERNAL "Internet connection not Found." FORCE)
      message(STATUS
        "\n------------------------------------------------------------------"
        "\nWarning:"
        "\n  Unable to establish Internet Connection. Some features may be"
        "\n  disabled. Fetching content from external sources has been" 
        "\n  disconnected and is offline."
        "\n------------------------------------------------------------------\n")

   else()
   
      if (NOT NETWORK_CHECK_RESULT_STATUS)
         message(STATUS "Internet Connection Found!\n")
         set(NETWORK_CHECK_RESULT_STATUS TRUE CACHE INTERNAL "Internet Found!\n" FORCE)
      endif()

   endif()    

endfunction()

function(require_internet_connection)
	check_for_internet_connection()
	if (NOT NETWORK_CHECK_RESULT_STATUS)
		message(STATUS "Internet connection is required but not available!")
		message(FATAL_ERROR)
	endif()
endfunction()