
# ---------------------------------------------------------
#                       Tests Module
# ---------------------------------------------------------


message(STATUS "Building tests")

set(AX_TEST_INC
   "${AX_PLAT_INCLUDES}"
   "${AX_SRC_ROOT}/src"
   "${AX_SRC_ROOT}/src/ident"
   "${AX_SRC_ROOT}/src/index"
   "${AX_SRC_ROOT}/src/quant"
   "${AX_SRC_ROOT}/src/audiocodes"
   "${AX_SRC_ROOT}/src/audio"
   "${AX_SRC_ROOT}/src/dbdrivers"
   "${AX_SRC_ROOT}/src/tools")


# --- Modules definitions ---

set(AX_TEST_FINGERPRINT_SRC test_fingerprinting.cpp
  ${AX_SRC_ROOT}/src/audio/AudioSource.cpp)
							 
set(AX_TEST_INDEXER_SRC test_indexing.cpp
  ${AX_SRC_ROOT}/src/audio/AudioSource.cpp
  ${AX_SRC_ROOT}/src/dbdrivers/${DATASTORE_T}.cpp)
							
set(AX_TEST_MATCHER_SRC test_matching.cpp
  ${AX_SRC_ROOT}/src/audio/AudioSource.cpp
  ${AX_SRC_ROOT}/src/dbdrivers/${DATASTORE_T}.cpp)


# --- Find targets libraries ---

find_library(DATASTORE_LIB
             NAMES ${AX_DATASTORE_LIB_NAME} 
             PATHS ${AX_TEST_LIB_PATHS})

# --- Setup the targets ---

add_executable(test_fingerprinting ${AX_TEST_FINGERPRINT_SRC})

target_include_directories(test_fingerprinting PRIVATE ${AX_TEST_INC})
target_compile_options(test_fingerprinting PRIVATE "${AX_TEST_CXX_FLAGS}")
target_compile_definitions(test_fingerprinting PRIVATE ${AX_TEST_DEFS})
target_link_libraries(test_fingerprinting audioneex
                      ${Boost_LIBRARIES} 
                      ${AX_PLAT_THREAD_LIB})

add_executable(test_indexing ${AX_TEST_INDEXER_SRC})

target_include_directories(test_indexing PRIVATE ${AX_TEST_INC})
target_compile_options(test_indexing PRIVATE "${AX_TEST_CXX_FLAGS}")
target_compile_definitions(test_indexing PRIVATE ${AX_TEST_DEFS})
target_link_libraries(test_indexing audioneex
                      ${Boost_LIBRARIES} 
                      ${AX_PLAT_THREAD_LIB} 
                      ${DATASTORE_LIB})

add_executable(test_matching ${AX_TEST_MATCHER_SRC})

target_include_directories(test_matching PRIVATE ${AX_TEST_INC})
target_compile_options(test_matching PRIVATE "${AX_TEST_CXX_FLAGS}")
target_compile_definitions(test_matching PRIVATE ${AX_TEST_DEFS})
target_link_libraries(test_matching audioneex
                      ${Boost_LIBRARIES} 
                      ${AX_PLAT_THREAD_LIB} 
                      ${DATASTORE_LIB})

foreach(TEST test_fingerprinting 
             test_indexing 
             test_matching)

    set_target_properties(${TEST} 
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${AX_BUILD_EXE_DIR}"
    )
endforeach()

