#
#  Audioneex cross-platform build script
#  -------------------------------------
#
#  Usage (Linux)  : cmake [-G "Unix Makefiles"][options] ..
#        (Windows): cmake -G "NMake Makefiles" [options] ..
#        (Android): cmake -DCMAKE_TOOLCHAIN_FILE=<path_to_toolchain_file>
#                         -DANDROID_NDK=<path_to_NDK_root_dir>
#                         -DANDROID_ABI=<architecture_name>
#                         -DANDROID_PLATFORM=<platform_version_number>
#                         -DANDROID_TOOLCHAIN=clang 
#                         [options] ..
#
#  where [options] is one or more of the following
#
#  AX_ARCH           = x32 | x64 | armeabi-v7a | arm64-v8a | x86 | x86_64
#  AX_API            = API version number (for Android only)
#  AX_BINARY_TYPE    = dynamic | static
#  AX_BUILD_MODE     = debug | release
#  AX_DATASTORE_T    = TCDataStore | CBDataStore
#  AX_WITH_EXAMPLES  = ON | OFF
#  AX_WITH_ID3       = ON | OFF  (for the examples only)
#  AX_WITH_TESTS     = ON | OFF  (for project developers only)
#

cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project (audioneex VERSION 1.3.0 LANGUAGES CXX)

enable_testing()

# =========================================================
#                      User Config
# =========================================================


set(MY_INCLUDE_DIRS
    
    /usr/local/include
    /usr/local/include/tcabinet
    
    C:/dev/cpp/include
)

set(MY_LIBRARY_x32_RELEASE_DIRS
)

set(MY_LIBRARY_x64_RELEASE_DIRS

    /usr/local/lib/7
    
    C:/dev/cpp/lib/vc191/x64/release
    C:/dev/cpp/lib/vc191/x64/release/static
)

set(MY_LIBRARY_x32_DEBUG_DIRS
)

set(MY_LIBRARY_x64_DEBUG_DIRS

    /usr/local/lib/7
    
    C:/dev/cpp/lib/vc191/x64/debug
    C:/dev/cpp/lib/vc191/x64/debug/static
)

set(MY_LIBRARY_ANDROID_RELEASE_DIRS

    /opt/android/lib/armeabi-v7a
    /opt/android/lib/arm64-v8a
    /opt/android/lib/x86
    /opt/android/lib/x86_64
    
    C:/dev/android/lib/x86_64
)

set(MY_LIBRARY_ANDROID_DEBUG_DIRS
)

set(MY_BOOST_INCLUDE_DIR

    /use/local/share/boost_1_66_0
)

set(MY_BOOST_LIBRARY_x32_DIR
)

set(MY_BOOST_LIBRARY_x64_DIR

    /usr/local/share/boost_1_66_0/stage/lib/7
)

set(MY_VC_INSTALL_DIR 

    "C:/Program Files (x86)/Microsoft Visual Studio"
)

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)


# ---------------------------------------------------------
#                  Build Configuration
# ---------------------------------------------------------


set(AX_SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Supported compilers
# 
# NOTE: These are the compilers that have been extensively
# tested and guaranteed to work. Others may work as well.
list(APPEND AX_SUPPORTED_COMPILERS 
     gnu clang msvc
)

list(APPEND AX_SUPPORTED_TARGETS
     linux windows android
)

list(APPEND AX_SUPPORTED_ARCHS
     x32 x64 armeabi-v7a arm64-v8a x86 x86_64
)

list(APPEND AX_SUPPORTED_DATABASE
     TCDataStore CBDataStore
)

list(APPEND AX_SUPPORTED_BINARYTYPE
     static dynamic
)

list(APPEND AX_SUPPORTED_BUILDMODES
     debug release
)


# ---------------------------------------------------------
#                   Functions and macros
# ---------------------------------------------------------


function(detect_target)
   string(TOLOWER "${CMAKE_SYSTEM_NAME}" _TARGET)
   if(_TARGET)
      set(AX_TARGET ${_TARGET} PARENT_SCOPE)
   else()
      message(FATAL_ERROR "\nERROR: No target platform detected.\n")
   endif()
endfunction()


function(detect_arch)
   if(AX_TARGET STREQUAL linux)
      if(AX_ARCH)
         set(_ARCH ${AX_ARCH})
      else()
         set(_ARCH x64)
      endif()
      set(AX_DETECTED_ARCH ${_ARCH} PARENT_SCOPE)
   elseif(AX_TARGET STREQUAL windows)
      execute_process (
          COMMAND cl
          ERROR_VARIABLE CL_CMD_ERROR
          OUTPUT_QUIET
      )
      string(REGEX MATCH "[23468xamdr_]+\n" _ARCH "${CL_CMD_ERROR}")
      if(NOT _ARCH)
         message(FATAL_ERROR "\nERROR: No architecture detected.\n")
      endif()
      string(REPLACE "\n" "" _ARCH "${_ARCH}")
      set(AX_DETECTED_ARCH ${_ARCH} PARENT_SCOPE)
   elseif(AX_TARGET STREQUAL android)
      set(AX_DETECTED_ARCH ${ANDROID_ABI} PARENT_SCOPE)
   endif()
endfunction()


function(detect_api)
   if(AX_TARGET STREQUAL linux)
      set(AX_DETECTED_API "N/A" PARENT_SCOPE)
   elseif(AX_TARGET STREQUAL windows)
      set(AX_DETECTED_API "N/A" PARENT_SCOPE)
   elseif(AX_TARGET STREQUAL android)
      if(ANDROID_PLATFORM)
         set(AX_DETECTED_API ${ANDROID_PLATFORM} PARENT_SCOPE)
      else()
         message(FATAL_ERROR "\nERROR: No Android API detected.\n")
      endif()
   endif()
endfunction()


function(show_param_usage PARAM_NAME PARAM_VAL PARAM_EXP)
   string(REPLACE ";" " | " _PARAM_FORMATTED_LIST "${PARAM_EXP}")
   message(FATAL_ERROR 
       "\nERROR: Invalid ${PARAM_NAME} parameter '${PARAM_VAL}'.\n"
       "Expected ${PARAM_NAME}=${_PARAM_FORMATTED_LIST}\n"
   )
endfunction()


# Default settings
# ----------------

detect_target()
detect_arch()
detect_api()

if(NOT AX_ARCH)
   set(AX_ARCH ${AX_DETECTED_ARCH})
endif()

if(NOT AX_API)
   set(AX_API ${AX_DETECTED_API})
endif()

if(NOT AX_BINARY_TYPE)
  set(AX_BINARY_TYPE dynamic)
endif()

if(NOT AX_BUILD_MODE)
  set(AX_BUILD_MODE release)
endif()

if(NOT DATASTORE_T)
   set(DATASTORE_T TCDataStore)
endif()


# Parameters check
# ----------------

if(NOT AX_TARGET IN_LIST AX_SUPPORTED_TARGETS)
   show_param_usage("AX_TARGET" "${AX_TARGET}" 
                    "${AX_SUPPORTED_TARGETS}")
endif()

if(NOT AX_ARCH IN_LIST AX_SUPPORTED_ARCHS)
   show_param_usage("AX_ARCH" "${AX_ARCH}" 
                    "${AX_SUPPORTED_ARCHS}")
endif()

if(NOT DATASTORE_T IN_LIST AX_SUPPORTED_DATABASE)
   show_param_usage("DATASTORE_T" "${DATASTORE_T}" 
                    "${AX_SUPPORTED_DATABASE}")
endif()

if(NOT AX_BINARY_TYPE IN_LIST AX_SUPPORTED_BINARYTYPE)
   show_param_usage("AX_BINARY_TYPE" "${AX_BINARY_TYPE}" 
                    "${AX_SUPPORTED_BINARYTYPE}")

endif()

# Parameters must be consistent with the current configuration

if(NOT ${AX_ARCH} MATCHES ${AX_DETECTED_ARCH})
   message(FATAL_ERROR 
      "\nERROR: The given architecture '${AX_ARCH}' for target "
      "'${AX_TARGET}' does not match the one currently set in the toolchain "
      "(${AX_DETECTED_ARCH}). Either omit the AX_ARCH parameter or "
      "set the toolchain to the given architecture."
   )
endif()

if(NOT ${AX_API} MATCHES ${AX_DETECTED_API})
   message(FATAL_ERROR 
      "\nERROR: The given API '${AX_API}' for target '${AX_TARGET}' "
      "does not match the one currently set in the toolchain "
      "(${AX_DETECTED_API}). Either omit the AX_API parameter or "
      "set the toolchain to the given API."
   )
endif()

# ---

if(AX_BUILD_MODE STREQUAL debug)
   set(CMAKE_BUILD_TYPE DEBUG)
elseif(AX_BUILD_MODE STREQUAL release)
   set(CMAKE_BUILD_TYPE RELEASE)
else()
   show_param_usage("AX_BUILD_MODE" "${AX_BUILD_MODE}" 
                    "${AX_SUPPORTED_BUILDMODES}")
endif()


# Verify that the compiler being used is supported
# ------------------------------------------------

string(TOLOWER "${CMAKE_CXX_COMPILER_ID}" COMPILER)
string(REPLACE "." "" COMPILER_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
string(SUBSTRING "${COMPILER_VERSION}" 0 4 COMPILER_VERSION)

if(NOT COMPILER IN_LIST AX_SUPPORTED_COMPILERS)
    message(FATAL_ERROR 
            "\nERROR: Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
else()
    set(AX_TOOLCHAIN "${COMPILER}${COMPILER_VERSION}")
endif()


# Global settings inherited by all targets for all platforms
# ----------------------------------------------------------

set(AX_DIR_NAME ${AX_TARGET}-${AX_ARCH}-${AX_TOOLCHAIN})
set(AX_VERSION ${PROJECT_VERSION})
set(AX_BUILD_LIB_DIR "${AX_SRC_ROOT}/_build/${AX_DIR_NAME}/${AX_BUILD_MODE}")
set(AX_BUILD_EXE_DIR "${AX_BUILD_LIB_DIR}")
set(AX_INSTALL_LIB_DIR "${AX_SRC_ROOT}/lib/${AX_DIR_NAME}/${AX_BUILD_MODE}")


# Target-specific setup
# -----------------------

include("cmake/make_${AX_TARGET}.cmake")

message(STATUS "--------------------------")
message(STATUS "Target       : ${AX_TARGET}")
message(STATUS "Architecture : ${AX_ARCH}")
message(STATUS "Toolchain    : ${AX_TOOLCHAIN}")
message(STATUS "API          : ${AX_API}")
message(STATUS "Binary type  : ${AX_BINARY_TYPE}")
message(STATUS "Build mode   : ${AX_BUILD_MODE}")
message(STATUS "--------------------------")
message(STATUS "Binaries dir: ${AX_DIR_NAME}")


# Post-build install script
# -------------------------

if(ANDROID)
   set(AX_BIN_PREFIX ${ANDROID_TOOLCHAIN_PREFIX})
   set(AX_ANDROID_LIBC 
      "${ANDROID_NDK}/sources/cxx-stl/llvm-libc++/libs/${AX_ARCH}/libc++_shared.so")
else()
   set(AX_BIN_PREFIX "")
endif()

# Export the current environment.
# This code will be included by the install script
# when 'make install' is invoked, and by the test
# runner script when tests are executed.
file(WRITE _build/ax_export_env.cmake
   "set(AX_SRC_ROOT \"${AX_SRC_ROOT}\")\n"
   "set(AX_TARGET ${AX_TARGET})\n"
   "set(AX_ARCH ${AX_ARCH})\n"
   "set(AX_BUILD_LIB_DIR \"${AX_BUILD_LIB_DIR}\")\n"
   "set(AX_BUILD_EXE_DIR \"${AX_BUILD_EXE_DIR}\")\n"
   "set(AX_INSTALL_LIB_DIR \"${AX_INSTALL_LIB_DIR}\")\n"
   "set(AX_BIN_PREFIX \"${AX_BIN_PREFIX}\")\n"
   "set(AX_BINARY_TYPE ${AX_BINARY_TYPE})\n"
   "set(AX_LIB_FFTSS \"${AX_LIB_FFTSS}\")\n"
   "set(AX_ANDROID_LIBC \"${AX_ANDROID_LIBC}\")\n"
   "set(MY_VC_INSTALL_DIR \"${MY_VC_INSTALL_DIR}\")\n"
)

install(SCRIPT "cmake/install.cmake")


