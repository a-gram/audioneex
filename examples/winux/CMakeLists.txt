
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

set(FPAUDIO_SRCS fpaudio.cpp 
  ${AX_SRC_ROOT}/src/dbdrivers/${DATASTORE_T}.cpp 
  ${AX_SRC_ROOT}/src/audio/AudioSource.cpp)

set(FPIDX_SRCS fpidx.cpp 
  ${AX_SRC_ROOT}/src/dbdrivers/${DATASTORE_T}.cpp)

set(FILEID_SRCS fileid.cpp 
  ${AX_SRC_ROOT}/src/dbdrivers/${DATASTORE_T}.cpp 
  ${AX_SRC_ROOT}/src/audio/AudioSource.cpp)

set(OTAID_SRCS otaid.cpp 
  ${AX_SRC_ROOT}/src/dbdrivers/${DATASTORE_T}.cpp 
  ${AX_SRC_ROOT}/src/audio/AudioSource.cpp)

  
# --- Find targets libraries ---

find_library(AX_DATASTORE_LIB
             NAMES ${AX_DATASTORE_LIB_NAME} 
             PATHS ${AX_EXE_LIB_PATHS})

find_library(AX_ID3TAG_LIB
             NAMES ${AX_ID3TAG_LIB_NAME} 
             PATHS ${AX_EXE_LIB_PATHS})


# --- Setup the targets ---

add_executable(fpaudio ${FPAUDIO_SRCS})

target_include_directories(fpaudio PRIVATE ${AX_EXE_INC})
target_compile_options(fpaudio PRIVATE "${AX_EXE_CXX_FLAGS}")
target_compile_definitions(fpaudio PRIVATE ${AX_EXE_DEFS})
target_link_libraries(fpaudio audioneex
                      ${Boost_LIBRARIES} ${AX_PLAT_THREAD_LIB}
                      ${AX_DATASTORE_LIB} ${AX_ID3TAG_LIB})

add_executable(fpidx ${FPIDX_SRCS})

target_include_directories(fpidx PRIVATE ${AX_EXE_INC})
target_compile_options(fpidx PRIVATE "${AX_EXE_CXX_FLAGS}")
target_compile_definitions(fpidx PRIVATE ${AX_EXE_DEFS})
target_link_libraries(fpidx audioneex
                      ${Boost_LIBRARIES}
                      ${AX_DATASTORE_LIB} ${AX_ID3TAG_LIB})

add_executable(fileid ${FILEID_SRCS})

target_include_directories(fileid PRIVATE ${AX_EXE_INC})
target_compile_options(fileid PRIVATE "${AX_EXE_CXX_FLAGS}")
target_compile_definitions(fileid PRIVATE ${AX_EXE_DEFS})
target_link_libraries(fileid audioneex
                      ${Boost_LIBRARIES} ${AX_PLAT_THREAD_LIB}
                      ${AX_DATASTORE_LIB} ${AX_ID3TAG_LIB})

add_executable(otaid ${OTAID_SRCS})

target_include_directories(otaid PRIVATE ${AX_EXE_INC})
target_compile_options(otaid PRIVATE "${AX_EXE_CXX_FLAGS}")
target_compile_definitions(otaid PRIVATE ${AX_EXE_DEFS})
target_link_libraries(otaid audioneex
                      ${Boost_LIBRARIES} ${AX_PLAT_THREAD_LIB}
                      ${AX_DATASTORE_LIB} ${AX_ID3TAG_LIB})

foreach(EXAMPLE fpaudio fpidx fileid otaid)
    set_target_properties(${EXAMPLE} 
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${AX_BUILD_EXE_DIR}"
    )
endforeach()
