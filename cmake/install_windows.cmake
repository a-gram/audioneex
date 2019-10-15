#
# Windows binaries installation script
# ------------------------------------
#

function(execute_install)
   
   if(AX_BINARY_TYPE STREQUAL dynamic)
   
      file(GLOB BUILD_LIBS ${AX_BUILD_LIB_DIR}/audioneex.*)
      file(COPY ${BUILD_LIBS} DESTINATION ${AX_INSTALL_LIB_DIR} 
           FILES_MATCHING REGEX "(\\.dll|\\.lib)$")

   elseif(AX_BINARY_TYPE STREQUAL static)

      set(VC_SETENV "vcvarsall.bat")
      
      message(STATUS 
             "Searching for vcvarsall.bat script. This may take a while...")
      
      file(GLOB_RECURSE VC_SETENV_FOUND ${MY_VC_INSTALL_DIR}/${VC_SETENV})
      list(LENGTH VC_SETENV_FOUND VC_SETENV_FOUND_COUNT)
      
      if(VC_SETENV_FOUND_COUNT EQUAL 1)
         message("Found ${VC_SETENV} in ${VC_SETENV_FOUND}")
      endif()

      if(VC_SETENV_FOUND_COUNT EQUAL 0)
         string(CONCAT ERR_MSG
          "The ${VC_SETENV} script was not found under the directory "
          "'${MY_VC_INSTALL_DIR}'. "
          "If Visual Studio is not installed in that directory then you must "
          "specify its location by setting the MY_VC_INSTALL_DIR variable in "
          "the User Config section of CMakeLists in the root directory.\n")
         message(FATAL_ERROR ${ERR_MSG})
      endif()

      if(VC_SETENV_FOUND_COUNT GREATER 1)
         string(CONCAT ERR_MSG
          "Multiple ${VC_SETENV} scripts found under the directory "
          "'${MY_VC_INSTALL_DIR}'. "
          "If there are multiple versions of Visual Studio installed then "
          "you must specify which one to use by setting the MY_VC_INSTALL_DIR "
          "variable in the User Config section of the root CMake script.\n")
         message(FATAL_ERROR ${ERR_MSG})
      endif()

      execute_process (
          COMMAND 
          cmd /C ${VC_SETENV_FOUND} ${AX_ARCH} &&
          lib /OUT:${AX_INSTALL_LIB_DIR}/audioneex.lib 
                   ${AX_BUILD_LIB_DIR}/audioneex.lib ${AX_LIB_FFTSS}
          ERROR_VARIABLE CMD_ERROR
      )
      check_cmd(lib)
      
   else()
      message(FATAL_ERROR 
          "\nERROR: BUG: ${AX_BINARY_TYPE}"
      )
   endif()

endfunction()

