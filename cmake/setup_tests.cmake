#
# Tests setup script
# ------------------
#
# This script is called by the 'setup_tests' CMake target defined in the 
# 'tests' directory when the test programs are built. It's purpose is that
# of performing the initialization of the test environment by preparing the 
# test data and all the resources needed to run the tests, so it must be 
# executed prior to running the tests.
#

include(../ax_export_env.cmake)

message(STATUS "Initializing tests ...")

# Copy the test data into the output directory
file(GLOB TEST_DATA_FILES ${AX_SRC_ROOT}/tests/data/*)
file(COPY ${TEST_DATA_FILES} DESTINATION "${AX_BUILD_EXE_DIR}/data")

# The tests need the datastore shared library in order to run. On Windows, 
# try copying the .dll in the tests output directory just in case it is 
# not available for loading system-wide (very likely). We assume the .dll 
# is in the same directory as its import lib.
if(AX_TARGET STREQUAL windows)
   string(REGEX REPLACE "(.*)\\.lib$" "\\1.dll" DB_DLL "${AX_DATASTORE_LIB}")
   if(EXISTS "${DB_DLL}")
      file(COPY ${DB_DLL} DESTINATION "${AX_BUILD_EXE_DIR}")
   else()
      message(WARNING 
         "\nWARNING: The datastore shared library was not found. "
         "The tests may fail if they can't load it. Make sure it's "
         "present in a location reachable system-wide, or in the same "
         "folder as its import lib so it can be automatically found.")
   endif()
endif()


