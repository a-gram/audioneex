
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

set(AX_TEST_FINGERPRINTER_SRC test_fingerprinter.cpp
  ${AX_SRC_ROOT}/src/audio/AudioSource.cpp)
							 
set(AX_TEST_INDEXER_SRC test_indexer.cpp
  ${AX_SRC_ROOT}/src/audio/AudioSource.cpp
  ${AX_SRC_ROOT}/src/dbdrivers/${DATASTORE_T}.cpp)
							
set(AX_TEST_MATCHER_SRC test_matcher.cpp
  ${AX_SRC_ROOT}/src/audio/AudioSource.cpp
  ${AX_SRC_ROOT}/src/dbdrivers/${DATASTORE_T}.cpp)

set(AX_TEST_RECOGNIZER_SRC test_recognizer.cpp
  ${AX_SRC_ROOT}/src/audio/AudioSource.cpp
  ${AX_SRC_ROOT}/src/dbdrivers/${DATASTORE_T}.cpp)


# --- Find targets libraries ---

find_library(AX_DATASTORE_LIB
             NAMES ${AX_DATASTORE_LIB_NAME} 
             PATHS ${AX_TEST_LIB_PATHS})

# --- Setup the targets ---

add_executable(test_fingerprinter ${AX_TEST_FINGERPRINTER_SRC})

target_include_directories(test_fingerprinter PRIVATE ${AX_TEST_INC})
target_compile_options(test_fingerprinter PRIVATE "${AX_TEST_CXX_FLAGS}")
target_compile_definitions(test_fingerprinter PRIVATE ${AX_TEST_DEFS})
target_link_libraries(test_fingerprinter audioneex
                      ${Boost_LIBRARIES} 
                      ${AX_PLAT_THREAD_LIB})

add_executable(test_indexer ${AX_TEST_INDEXER_SRC})

target_include_directories(test_indexer PRIVATE ${AX_TEST_INC})
target_compile_options(test_indexer PRIVATE "${AX_TEST_CXX_FLAGS}")
target_compile_definitions(test_indexer PRIVATE ${AX_TEST_DEFS})
target_link_libraries(test_indexer audioneex
                      ${Boost_LIBRARIES} 
                      ${AX_PLAT_THREAD_LIB} 
                      ${AX_DATASTORE_LIB})

add_executable(test_matcher ${AX_TEST_MATCHER_SRC})

target_include_directories(test_matcher PRIVATE ${AX_TEST_INC})
target_compile_options(test_matcher PRIVATE "${AX_TEST_CXX_FLAGS}")
target_compile_definitions(test_matcher PRIVATE ${AX_TEST_DEFS})
target_link_libraries(test_matcher audioneex
                      ${Boost_LIBRARIES} 
                      ${AX_PLAT_THREAD_LIB} 
                      ${AX_DATASTORE_LIB})

add_executable(test_recognizer ${AX_TEST_RECOGNIZER_SRC})

target_include_directories(test_recognizer PRIVATE ${AX_TEST_INC})
target_compile_options(test_recognizer PRIVATE "${AX_TEST_CXX_FLAGS}")
target_compile_definitions(test_recognizer PRIVATE ${AX_TEST_DEFS})
target_link_libraries(test_recognizer audioneex
                      ${Boost_LIBRARIES} 
                      ${AX_PLAT_THREAD_LIB} 
                      ${AX_DATASTORE_LIB})

# Set common properties and create test targets
foreach(TEST test_fingerprinter 
             test_indexer 
             test_matcher
             test_recognizer)

   set_target_properties(${TEST} 
       PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${AX_BUILD_EXE_DIR}"
   )
   string(REPLACE "test_" "" TEST_NAME "${TEST}")
   add_test(
      NAME ${TEST_NAME}
      COMMAND ${TEST}
      WORKING_DIRECTORY "${AX_BUILD_EXE_DIR}"
   )
endforeach()

# Create a target to initialize the tests prior to execution. 
# See 'setup_tests.cmake' for details about what the setup process does.
add_custom_target(
   setup_tests
   ALL
   COMMAND ${CMAKE_COMMAND} -P ${AX_SRC_ROOT}/cmake/setup_tests.cmake
)

