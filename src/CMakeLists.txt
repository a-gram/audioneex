
# ---------------------------------------------------------
#                      Library Module
# ---------------------------------------------------------


set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(AX_LIB_INC
    ${AX_PLAT_INCLUDES}
    ${AX_SRC_ROOT}/src
    ${AX_SRC_ROOT}/src/ident
    ${AX_SRC_ROOT}/src/index
    ${AX_SRC_ROOT}/src/quant
    ${AX_SRC_ROOT}/src/audiocodes
    ${AX_SRC_ROOT}/src/audio
    ${AX_SRC_ROOT}/src/tools)

set(AX_LIB_SRC
    ident/Fingerprinter.cpp
    ident/Matcher.cpp
    ident/MatchFuzzyClassifier.cpp
    ident/Recognizer.cpp
    index/BlockCodec.cpp
    index/Indexer.cpp
    quant/Codebook.cpp
    audiocodes/AudioCodes.cpp)


if(AX_WITH_TESTS)
   set(AX_LIB_DEFS ${AX_LIB_DEFS} -DTESTING)
   string(CONCAT EMESSAGE
          "\nWARNING: The library is being built in TEST MODE, which "
          "is not meant for production. If you're not a project "
          "developer, rebuild without the AX_WITH_TESTS definition.")
   message(WARNING ${EMESSAGE})
endif()


# --- Check module-specific parameters ---

if(AX_BINARY_TYPE STREQUAL dynamic)
   set(AX_LIB_TYPE SHARED)
   set(AX_LIB_DEFS ${AX_LIB_DEFS} -DAUDIONEEX_DLL)
elseif(AX_BINARY_TYPE STREQUAL static)
   set(AX_LIB_TYPE STATIC)
else()
   message(FATAL_ERROR 
       "\nERROR: Invalid build type: ${AX_BINARY_TYPE}. Use static|dynamic."
   )
endif()

if(AX_LIB_VERSIONED)
   set(AX_LIB_VERSION VERSION ${AX_VERSION})
   set(AX_LIB_SOVERSION SOVERSION ${PROJECT_VERSION_MAJOR})
else()
   set(AX_LIB_VERSION "")
   set(AX_LIB_SOVERSION "")
endif()


# --- Find targets libraries ---

find_library(AX_LIB_FFTSS
             NAMES fftss
             PATHS ${AX_LIB_LIB_PATHS})


# --- Setup the targets ---

add_library(audioneex ${AX_LIB_TYPE} ${AX_LIB_SRC})
target_include_directories(audioneex PRIVATE ${AX_LIB_INC})
target_compile_options(audioneex PRIVATE ${AX_LIB_CXX_FLAGS})
target_compile_definitions(audioneex PRIVATE ${AX_LIB_DEFS})
target_link_libraries(audioneex ${AX_LIB_FFTSS})

set_target_properties(audioneex
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${AX_BUILD_LIB_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${AX_BUILD_LIB_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${AX_BUILD_LIB_DIR}"
    ${AX_LIB_VERSION}
    ${AX_LIB_SOVERSION}
)

