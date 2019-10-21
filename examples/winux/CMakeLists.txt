
# ---------------------------------------------------------
#                     Examples Module
# ---------------------------------------------------------


message(STATUS "Building examples")

set(AX_EXE_INC
   "${AX_PLAT_INCLUDES}"
   "${AX_SRC_ROOT}/src"
   "${AX_SRC_ROOT}/src/audio" 
   "${AX_SRC_ROOT}/src/dbdrivers"
   "${AX_SRC_ROOT}/src/tools")


# --- Modules definitions ---

set(EXAMPLE1_SRCS example1.cpp 
  ${AX_SRC_ROOT}/src/dbdrivers/${DATASTORE_T}.cpp 
  ${AX_SRC_ROOT}/src/audio/AudioSource.cpp)

set(EXAMPLE2_SRCS example2.cpp 
  ${AX_SRC_ROOT}/src/dbdrivers/${DATASTORE_T}.cpp)

set(EXAMPLE3_SRCS example3.cpp 
  ${AX_SRC_ROOT}/src/dbdrivers/${DATASTORE_T}.cpp 
  ${AX_SRC_ROOT}/src/audio/AudioSource.cpp)

set(EXAMPLE4_SRCS example4.cpp 
  ${AX_SRC_ROOT}/src/dbdrivers/${DATASTORE_T}.cpp 
  ${AX_SRC_ROOT}/src/audio/AudioSource.cpp)

  
# --- Find targets libraries ---

find_library(AX_DATASTORE_LIB
             NAMES ${AX_DATASTORE_LIB_NAME} 
             PATHS ${AX_EXE_LIB_PATHS})

if(AX_WITH_ID3)
find_library(AX_ID3TAG_LIB
             NAMES ${AX_ID3TAG_LIB_NAME} 
             PATHS ${AX_EXE_LIB_PATHS})
endif()


# --- Setup the targets ---

add_executable(example1 ${EXAMPLE1_SRCS})

target_include_directories(example1 PRIVATE ${AX_EXE_INC})
target_compile_options(example1 PRIVATE "${AX_EXE_CXX_FLAGS}")
target_compile_definitions(example1 PRIVATE ${AX_EXE_DEFS})
target_link_libraries(example1 audioneex
                      ${Boost_LIBRARIES} ${AX_PLAT_THREAD_LIB}
                      ${AX_DATASTORE_LIB} ${AX_ID3TAG_LIB})

add_executable(example2 ${EXAMPLE2_SRCS})

target_include_directories(example2 PRIVATE ${AX_EXE_INC})
target_compile_options(example2 PRIVATE "${AX_EXE_CXX_FLAGS}")
target_compile_definitions(example2 PRIVATE ${AX_EXE_DEFS})
target_link_libraries(example2 audioneex
                      ${Boost_LIBRARIES}
                      ${AX_DATASTORE_LIB} ${AX_ID3TAG_LIB})

add_executable(example3 ${EXAMPLE3_SRCS})

target_include_directories(example3 PRIVATE ${AX_EXE_INC})
target_compile_options(example3 PRIVATE "${AX_EXE_CXX_FLAGS}")
target_compile_definitions(example3 PRIVATE ${AX_EXE_DEFS})
target_link_libraries(example3 audioneex
                      ${Boost_LIBRARIES} ${AX_PLAT_THREAD_LIB}
                      ${AX_DATASTORE_LIB} ${AX_ID3TAG_LIB})

add_executable(example4 ${EXAMPLE4_SRCS})

target_include_directories(example4 PRIVATE ${AX_EXE_INC})
target_compile_options(example4 PRIVATE "${AX_EXE_CXX_FLAGS}")
target_compile_definitions(example4 PRIVATE ${AX_EXE_DEFS})
target_link_libraries(example4 audioneex
                      ${Boost_LIBRARIES} ${AX_PLAT_THREAD_LIB}
                      ${AX_DATASTORE_LIB} ${AX_ID3TAG_LIB})

foreach(EXAMPLE example1 example2 example3 example4)
    set_target_properties(${EXAMPLE} 
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${AX_BUILD_EXE_DIR}"
    )
endforeach()
