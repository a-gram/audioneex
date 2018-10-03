
# ---------------------------------------------------------
#                       Tests Module
# ---------------------------------------------------------

message(STATUS "Building tests")

include_directories(${AX_SRC_ROOT}/src
                    ${AX_SRC_ROOT}/src/ident
                    ${AX_SRC_ROOT}/src/index
                    ${AX_SRC_ROOT}/src/quant
                    ${AX_SRC_ROOT}/src/audiocodes
                    ${AX_SRC_ROOT}/audio
                    ${AX_SRC_ROOT}/DAO
                    ${AX_SRC_ROOT}/tools)

set(AX_TEST_FINGERPRINT_SRC test_fingerprinting.cpp
                            ${AX_SRC_ROOT}/audio/AudioSource.cpp)
							 
set(AX_TEST_INDEXER_SRC     test_indexing.cpp
                            ${AX_SRC_ROOT}/audio/AudioSource.cpp
                            ${AX_SRC_ROOT}/DAO/${DATASTORE_T}.cpp)
							
set(AX_TEST_MATCHER_SRC     test_matching.cpp
                            ${AX_SRC_ROOT}/audio/AudioSource.cpp
                            ${AX_SRC_ROOT}/DAO/${DATASTORE_T}.cpp)


# --- Platform-specific module compilation settings ---

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

    set(TEST_CXX_FLAGS "")
	set(THREAD_LIB pthread)
	
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

    set(TEST_CXX_FLAGS "/Od")

endif()

# --- Check for required libraries ---

find_library(DATASTORE_LIB
             NAMES ${DATASTORE_LIB_NAME} 
             PATHS ${MY_LIBRARY_${ARCH}_${CMAKE_BUILD_TYPE}_DIRS})

# --- Build the modules ---

add_executable(test_fingerprinting ${AX_TEST_FINGERPRINT_SRC})
target_compile_options(test_fingerprinting PRIVATE "${TEST_CXX_FLAGS}")
target_link_libraries(test_fingerprinting audioneex
                      ${Boost_LIBRARIES} ${THREAD_LIB})

add_executable(test_indexing ${AX_TEST_INDEXER_SRC})
target_compile_options(test_indexing PRIVATE "${TEST_CXX_FLAGS}")
target_link_libraries(test_indexing audioneex
                      ${Boost_LIBRARIES} ${THREAD_LIB} ${DATASTORE_LIB})

add_executable(test_matching ${AX_TEST_MATCHER_SRC})
target_compile_options(test_matching PRIVATE "${TEST_CXX_FLAGS}")
target_link_libraries(test_matching audioneex
                      ${Boost_LIBRARIES} ${THREAD_LIB} ${DATASTORE_LIB})

