
# ===========================
#  Linux build configuration
# ===========================

# This script defines platform-level build configuration specific for Android. 
# It defines AX_PLAT_* variables that apply globally to all targets, and
# target variables (AX_LIB_*, AX_EXE_, etc.) that apply to specific types and
# that will be "inherited" for more "specialization" by the defined targets.

# Setting the following to have CMake find stuff outside the paths
# set by the NDK does not seem to work. We have to completely unset.
# set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

unset(CMAKE_FIND_ROOT_PATH)
unset(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE)
unset(CMAKE_SYSROOT)


# Global configuration
# --------------------

set(AX_PLAT_INCLUDES ${MY_INCLUDE_DIRS})
set(AX_PLAT_DEFS "")
set(AX_PLAT_CXX_FLAGS -std=c++11 -fexceptions)
set(AX_PLAT_LIB_PATHS ${MY_LIBRARY_ANDROID_${CMAKE_BUILD_TYPE}_DIRS})
list(FILTER AX_PLAT_LIB_PATHS INCLUDE REGEX "/${AX_ARCH}$")


# Library configuration
# ---------------------

set(AX_LIB_DEFS ${AX_PLAT_DEFS} -DNDEBUG -DAUDIONEEX_API_EXPORT)
set(AX_LIB_LIB_PATHS ${AX_PLAT_LIB_PATHS})
set(AX_LIB_VERSIONED TRUE)


# Find project's required libraries
# ---------------------------------

find_package(Boost 1.55)

# Try the user settings if Boost is not found in BOOST_ROOT.
if(NOT Boost_FOUND)
   message(STATUS 
       "[!] Boost not found in BOOST_ROOT. Trying user's paths ..."
   )
   set(BOOST_INCLUDEDIR ${MY_BOOST_INCLUDE_DIR})
   find_package(Boost 1.55 REQUIRED)
endif()

set(AX_PLAT_INCLUDES ${AX_PLAT_INCLUDES} ${Boost_INCLUDE_DIRS})

add_subdirectory(src)


