#
# Installation script
# -------------------
#
# This script installs the final binaries produced by the build into a
# 'lib' directory in the root and applies some manipulation in order to
# optimize and reduce the size of the binaries. Target-specific installation 
# is implemented inside 'install_<target>.cmake' scripts, which are included 
# by this one. It also includes the necessary build environment to perform 
# the installation.
#

include(ax_export_env.cmake)
include(../cmake/install_${AX_TARGET}.cmake)

function(check_cmd cmd_name)
   if(NOT CMD_ERROR STREQUAL "")
      message(FATAL_ERROR
          "\nERROR: '${cmd_name}' failed with error: ${CMD_ERROR}")
   endif()
endfunction()

set(AX_PWD "${CMAKE_CURRENT_SOURCE_DIR}")
file(MAKE_DIRECTORY "${AX_INSTALL_LIB_DIR}")

execute_install()

message(STATUS "Installation succeeded.")
message(STATUS "Binaries can be found in '${AX_INSTALL_LIB_DIR}'")

