#
# Linux binaries installation script
# ----------------------------------
#

function(execute_install)
   
   set(AX_I_SLIB "${AX_INSTALL_LIB_DIR}/libaudioneex.a")
   set(AX_I_DLIB "${AX_INSTALL_LIB_DIR}/libaudioneex.so")
   set(AX_B_SLIB "${AX_BUILD_LIB_DIR}/libaudioneex.a")
   set(AX_B_DLIB "${AX_BUILD_LIB_DIR}/libaudioneex.so")

   set(AX_AR_CMD "${AX_BIN_PREFIX}ar")
   set(AX_STRIP_CMD "${AX_BIN_PREFIX}strip")

   if(AX_BINARY_TYPE STREQUAL dynamic)

      file(GLOB BUILD_LIBS ${AX_BUILD_LIB_DIR}/lib*)
      file(COPY ${BUILD_LIBS} DESTINATION ${AX_INSTALL_LIB_DIR})

      execute_process (
          COMMAND ${AX_STRIP_CMD} --strip-unneeded ${AX_I_DLIB}
          ERROR_VARIABLE CMD_ERROR
      )
      check_cmd(copy-strip)

   elseif(AX_BINARY_TYPE STREQUAL static)

      set(LIB1_OBJ_DIR "tmp060975/lib1_o")
      set(LIB2_OBJ_DIR "tmp060975/lib2_o")

      file(MAKE_DIRECTORY "${LIB1_OBJ_DIR}" "${LIB2_OBJ_DIR}")
      
      execute_process (
          COMMAND ${AX_AR_CMD} -x ${AX_B_SLIB}
          WORKING_DIRECTORY "${LIB1_OBJ_DIR}"
          ERROR_VARIABLE CMD_ERROR
      )
      check_cmd(unpack1)
      
      execute_process (
          COMMAND ${AX_AR_CMD} -x ${AX_LIB_FFTSS}
          WORKING_DIRECTORY "${LIB2_OBJ_DIR}"
          ERROR_VARIABLE CMD_ERROR
      )
      check_cmd(unpack2)

      file(GLOB_RECURSE LIB_OBJS tmp060975/*.o)
      
      execute_process (
          COMMAND ${AX_AR_CMD} -rc ${AX_I_SLIB} ${LIB_OBJS}
          ERROR_VARIABLE CMD_ERROR
      )
      check_cmd(pack)

   else()
      message(FATAL_ERROR 
          "\nERROR: BUG: ${AX_BINARY_TYPE}"
      )
   endif()

endfunction()

