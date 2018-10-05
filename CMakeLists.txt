#
#  Audioneex build script.
#
#  Usage (Linux)  : cmake [options] ..
#        (Windows): cmake -G "NMake Makefiles" [options] ..
#
#  where [options] is one or more of the following parameters
#
#  ARCH            = x32|x64
#  TOOLCHAIN       = gcc64|vc14|...
#  BINARY_TYPE     = dynamic|static
#  BUILD_MODE      = debug|release
#  WITH_EXAMPLES   = ON|OFF
#  DATASTORE_T     = TCDataStore|CBDataStore
#  ID3_TAG_SUPPORT = ON|OFF  (for the examples only)
#  WITH_TESTS      = ON|OFF  (for project developers only)
#

cmake_minimum_required(VERSION 3.5.0 FATAL_ERROR)

project (audioneex VERSION 1.2.0 LANGUAGES CXX)


# ---------------------------------------------------------
#                   User Configuration
# ---------------------------------------------------------


# Most include and library paths are automatically found by
# cmake. Should it fail to find something, in this section it
# is possible to set useful paths according to the current user's
# environment, which will be passed on to cmake as hints.

list(APPEND MY_INCLUDE_DIRS
    
    "c:/dev/cpp/include"
    "/usr/local/include/tcabinet"

)

list(APPEND MY_LIBRARY_x32_RELEASE_DIRS
)

list(APPEND MY_LIBRARY_x64_RELEASE_DIRS

    "c:/dev/cpp/lib/fftss/x64/vc141/release"
    "c:/dev/cpp/lib/ejdb/x64/vc141/release"
	"c:/dev/cpp/lib/libcouchbase/x64/vc141/release"
    "c:/dev/cpp/lib/taglib/x64/vc141/release"
    "/usr/local/lib/7"

)

list(APPEND MY_LIBRARY_x32_DEBUG_DIRS
)

list(APPEND MY_LIBRARY_x64_DEBUG_DIRS
)

set(MY_BOOST_INCLUDE_DIR

    "/usr/local/share/boost_1_66_0"
)

set(MY_BOOST_LIBRARY_x32_DIR
)

set(MY_BOOST_LIBRARY_x64_DIR

    "/usr/local/share/boost_1_66_0/stage/lib/7"
)

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)


# ---------------------------------------------------------
#                  Build Configuration
# ---------------------------------------------------------


set(AX_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR})


# ----------------- Environment settings ------------------


# Default setting
if(NOT BINARY_TYPE)
  set(BINARY_TYPE dynamic)
endif()

if(NOT BUILD_MODE)
  set(BUILD_MODE release)
endif()

if(NOT ARCH)
   set(ARCH x64)
endif()


if(BUILD_MODE STREQUAL "debug")
   set(CMAKE_BUILD_TYPE DEBUG)
elseif(BUILD_MODE STREQUAL "release")
   set(CMAKE_BUILD_TYPE RELEASE)
else()
   message(FATAL_ERROR "\nERROR: Unsupported build mode '${BUILD_MODE}'. Use -BUILD_MODE=debug|release")
endif()

if(NOT ARCH STREQUAL "x32" AND NOT ARCH STREQUAL "x64")
   message(FATAL_ERROR "\nERROR: Unsupported architecture '${ARCH}'. Use -DARCH=x32|x64")
endif()


if(NOT DATASTORE_T)
   set(DATASTORE_T "TCDataStore")
endif()

if(DATASTORE_T STREQUAL "TCDataStore")
   # On Windows some libraries have different namings.
   if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      set(DATASTORE_LIB_NAME libejdb)
   else()
      set(DATASTORE_LIB_NAME tokyocabinet)
   endif()
   add_definitions(-DDATASTORE_T_ID=1)
   add_definitions(-DDATASTORE_T=${DATASTORE_T})
elseif(DATASTORE_T STREQUAL "CBDataStore")
   # On Windows some libraries have different namings.
   if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      set(DATASTORE_LIB_NAME libcouchbase)
   else()
      set(DATASTORE_LIB_NAME couchbase)
   endif()
   add_definitions(-DDATASTORE_T_ID=2)
   add_definitions(-DDATASTORE_T=${DATASTORE_T})
else()
   message(FATAL_ERROR "\nERROR: Unsupported data store.")
endif()

if(ID3_TAG_SUPPORT)
   set(ID3TAG_LIB_NAME tag)
   add_definitions(-DID3_TAG_SUPPORT)
endif()

# Set project's include paths

foreach (dir ${MY_INCLUDE_DIRS})
    include_directories("${dir}")
endforeach()

# Platform-specific global compilation settings (all modules)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

   if(NOT TOOLCHAIN)
      set(TOOLCHAIN gcc72)
   endif()

   set(COMP_FLAGS "-std=c++11 -fexceptions")
   
   if(ARCH STREQUAL "x64")
      set(COMP_FLAGS "${COMP_FLAGS} -m64")
   else()      
      set(COMP_FLAGS "${COMP_FLAGS} -m32")
   endif()
   
   if(CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 6.2)
      set(COMP_FLAGS "${COMP_FLAGS} -no-pie")
   endif()
   
   # Build-specific flags
   if(BUILD_MODE STREQUAL "debug")
      set(COMP_FLAGS "${COMP_FLAGS} -Wall -g -Og")
   elseif(BUILD_MODE STREQUAL "release")
      set(COMP_FLAGS "${COMP_FLAGS} -Wno-deprecated-declarations")
   endif()

   set(PLATFORM linux)
   
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

   if(NOT TOOLCHAIN)
     set(TOOLCHAIN vc141)
   endif()

   add_definitions(-DWIN32)
   add_definitions(-DWIN32_LEAN_AND_MEAN) # Fix winsock.h clashes
   
   set(COMP_FLAGS "/WX- /EHsc /Gd")
   
   # Build-specific flags
   if(BUILD_MODE STREQUAL "debug")
      set(COMP_FLAGS "${COMP_FLAGS} /W3 /RTC1 /Zi /MDd")
   elseif(BUILD_MODE STREQUAL "release")
      set(COMP_FLAGS "${COMP_FLAGS} /W1 /MD")
   endif()
   
   set(PLATFORM win)
   
else()
   message(FATAL_ERROR "\nERROR: Unsupported compiler")
endif()

set(CMAKE_CXX_FLAGS ${COMP_FLAGS})

set(AX_VERSION ${PROJECT_VERSION})
set(AX_OUTPUT_LIB_DIR "${AX_SRC_ROOT}/build/${PLATFORM}-${ARCH}-${TOOLCHAIN}/${BUILD_MODE}")
set(AX_OUTPUT_EXE_DIR "${AX_SRC_ROOT}/build/${PLATFORM}-${ARCH}-${TOOLCHAIN}-bin")
set(AX_POST_BUILD_SCRIPT_ARGS ${PLATFORM} ${ARCH} ${TOOLCHAIN} ${BUILD_MODE} ${BINARY_TYPE})

# Find project's required libraries

find_package(Boost 1.55 COMPONENTS filesystem system thread regex)

# Try the user settings if Boost is not found in BOOST_ROOT.
if(NOT Boost_FOUND)
   message (STATUS "[!] Boost not found in BOOST_ROOT. Trying user's paths.")
   set(BOOST_INCLUDEDIR ${MY_BOOST_INCLUDE_DIR})
   set(BOOST_LIBRARYDIR ${MY_BOOST_LIBRARY_${ARCH}_DIR})
   find_package(Boost 1.55 REQUIRED COMPONENTS filesystem system thread regex)
endif()

if(Boost_FOUND)
   include_directories(${Boost_INCLUDE_DIRS})
endif()

# Build the project

message(STATUS "Building for ${PLATFORM}-${ARCH}-${TOOLCHAIN} (${CMAKE_BUILD_TYPE}) ...")

add_subdirectory(src)

if(WITH_EXAMPLES)
   add_subdirectory(examples/winux)
endif()

if(WITH_TESTS)
   add_subdirectory(tests)
endif()


# Post-build script (note that this is executed after the audioneex target has
# been built, not after the whole project build)
add_custom_target(buildlib ALL
                  COMMAND ${AX_SRC_ROOT}/build_dist_libs ${AX_POST_BUILD_SCRIPT_ARGS}
                  WORKING_DIRECTORY ${AX_SRC_ROOT}
                  DEPENDS audioneex)


