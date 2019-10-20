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

file(GLOB TEST_DATA_FILES ${AX_SRC_ROOT}/tests/data/*)
file(COPY ${TEST_DATA_FILES} DESTINATION "${AX_BUILD_EXE_DIR}/data")

