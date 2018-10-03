
# ---------------------------------------------------------
#                      Library Module
# ---------------------------------------------------------

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_definitions(-DNDEBUG -DAUDIONEEX_API_EXPORT)

include_directories(${AX_SRC_ROOT}/src
                    ${AX_SRC_ROOT}/src/ident
                    ${AX_SRC_ROOT}/src/index
                    ${AX_SRC_ROOT}/src/quant
                    ${AX_SRC_ROOT}/src/audiocodes
                    ${AX_SRC_ROOT}/audio
                    ${AX_SRC_ROOT}/tools)

set(AX_LIB_SRC
    ident/Fingerprint.cpp
    ident/Matcher.cpp
    ident/Recognizer.cpp
    index/BlockCodec.cpp
    index/Indexer.cpp
    quant/Codebook.cpp
    audiocodes/AudioCodes.cpp)


if(WITH_TESTS)
   add_definitions(-DTESTING)
   message(WARNING "\nWARNING: The library is being built in TEST MODE, which is not meant for release. \
                    If you're not a project developer, rebuild without the WITH_TESTS definition.")
endif()

# --- Check module-specific parameters ---

if(BINARY_TYPE STREQUAL "dynamic")
   set(BUILD_SHARED_LIBS ON)
   add_definitions(-DAUDIONEEX_DLL)
   message(STATUS "Building dynamic library")
elseif(BINARY_TYPE STREQUAL "static")
   set(BUILD_SHARED_LIBS OFF)
   message(STATUS "Building static library")
else()
   message(FATAL_ERROR "\nERROR: Unsupported build type '${BINARY_TYPE}'. Use -DBINARY_TYPE=static|dynamic.")
endif()

# --- Platform-specific module compilation settings ---

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

   set(LIB_CXX_FLAGS -fvisibility=hidden -fno-rtti)

   if(BUILD_MODE STREQUAL "debug")
      set(LIB_CXX_FLAGS ${LIB_CXX_FLAGS})
   elseif(BUILD_MODE STREQUAL "release")
      set(LIB_CXX_FLAGS ${LIB_CXX_FLAGS} -O2)
   endif()
	
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

   set(LIB_CXX_FLAGS "")
   
   if(BUILD_MODE STREQUAL "debug")
      set(LIB_CXX_FLAGS ${LIB_CXX_FLAGS} /GR- /Od)
   elseif(BUILD_MODE STREQUAL "release")
      set(LIB_CXX_FLAGS ${LIB_CXX_FLAGS} /GS- /GR- /O2)
   endif()
	
endif()

# --- Check for required libraries ---

find_library(FFTSS_LIB
             NAMES fftss
             PATHS ${MY_LIBRARY_${ARCH}_${CMAKE_BUILD_TYPE}_DIRS})

# --- Build the modules ---

add_library(audioneex ${AX_LIB_SRC})
target_compile_options(audioneex PRIVATE ${LIB_CXX_FLAGS})
target_link_libraries(audioneex ${FFTSS_LIB})

set_target_properties(audioneex
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${AX_OUTPUT_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${AX_OUTPUT_LIB_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${AX_OUTPUT_LIB_DIR}"
    VERSION ${AX_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)


