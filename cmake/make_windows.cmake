
# =============================
#  Windows build configuration
# =============================

# This script defines platform-level build configuration specific for Windows. 
# It defines AX_PLAT_* variables that apply globally to all targets, and
# target variables (AX_LIB_*, AX_EXE_, etc.) that apply to specific types and
# that will be "inherited" for more "specialization" by the defined targets.


# Global configuration
# --------------------

set(AX_PLAT_INCLUDES ${MY_INCLUDE_DIRS})
set(AX_PLAT_DEFS "")
set(AX_PLAT_CXX_FLAGS /WX- /EHsc /Gd)
set(AX_PLAT_LIB_PATHS ${MY_LIBRARY_${AX_ARCH}_${CMAKE_BUILD_TYPE}_DIRS})
set(AX_PLAT_THREAD_LIB "")

if(AX_BUILD_MODE STREQUAL debug)
   set(AX_PLAT_CXX_FLAGS ${AX_PLAT_CXX_FLAGS} /W3 /RTC1 /Zi /MDd)
elseif(AX_BUILD_MODE STREQUAL release)
   set(AX_PLAT_CXX_FLAGS ${AX_PLAT_CXX_FLAGS} /W1 /MD)
endif()

if(AX_NO_RTTI)
   set(AX_PLAT_CXX_FLAGS ${AX_PLAT_CXX_FLAGS} /GR-)
endif()

if(DATASTORE_T STREQUAL TCDataStore)
   set(AX_DATASTORE_LIB_NAME libejdb)
   set(AX_PLAT_DEFS ${AX_PLAT_DEFS} 
       -DDATASTORE_T_ID=1 -DDATASTORE_T=${DATASTORE_T})
elseif(DATASTORE_T STREQUAL CBDataStore)
   set(AX_DATASTORE_LIB_NAME libcouchbase)
   set(AX_PLAT_DEFS ${AX_PLAT_DEFS} 
       -DDATASTORE_T_ID=2 -DDATASTORE_T=${DATASTORE_T})
else()
   message(FATAL_ERROR "Bug: ${DATASTORE_T}")
endif()


# Library configuration
# ---------------------

set(AX_LIB_DEFS ${AX_PLAT_DEFS} -DNDEBUG -DAUDIONEEX_API_EXPORT)

if(AX_BUILD_MODE STREQUAL debug)
   set(AX_LIB_CXX_FLAGS ${AX_PLAT_CXX_FLAGS} /Od)
elseif(AX_BUILD_MODE STREQUAL release)
   set(AX_LIB_CXX_FLAGS ${AX_PLAT_CXX_FLAGS} /GS- /O2)
endif()

set(AX_LIB_LIB_PATHS ${AX_PLAT_LIB_PATHS})
set(AX_LIB_VERSIONED TRUE)


# Examples configuration
# ----------------------

set(AX_EXE_DEFS ${AX_PLAT_DEFS} 
    -DWIN32 -DWIN32_LEAN_AND_MEAN -DWITH_ID3
) #         ^--Fix winsock clashes
set(AX_EXE_CXX_FLAGS ${AX_PLAT_CXX_FLAGS})
set(AX_EXE_LIB_PATHS ${AX_PLAT_LIB_PATHS})
set(AX_ID3TAG_LIB_NAME tag)


# Tests configuration
# -------------------

set(AX_TEST_DEFS ${AX_PLAT_DEFS} -DDATASTORE_T_ID=1)
set(AX_TEST_CXX_FLAGS ${AX_PLAT_CXX_FLAGS} /Od) # (*)
set(AX_TEST_LIB_PATHS ${AX_PLAT_LIB_PATHS})

# (*) Apparently we have to disable optimizations when using the library 
#     compiled in TEST mode, otherwise it crashes (at least on MSVC 19.13)

    
# Project-level libraries
# -----------------------

find_package(Boost 1.55 COMPONENTS 
             filesystem system thread regex)

# Try the user settings if Boost is not found in BOOST_ROOT.
if(NOT Boost_FOUND)
   message(STATUS 
       "[!] Boost not found in BOOST_ROOT. Trying user's paths ..."
   )
   set(BOOST_INCLUDEDIR ${MY_BOOST_INCLUDE_DIR})
   set(BOOST_LIBRARYDIR ${MY_BOOST_LIBRARY_${AX_ARCH}_DIR})
   find_package(Boost 1.55 REQUIRED COMPONENTS 
                filesystem system thread regex)
endif()

set(AX_PLAT_INCLUDES ${AX_PLAT_INCLUDES} ${Boost_INCLUDE_DIRS})


# Create the build tree
# ---------------------

add_subdirectory(src)

if(AX_WITH_EXAMPLES)
   add_subdirectory(examples/winux)
endif()

if(AX_WITH_TESTS)
   add_subdirectory(tests)
endif()

