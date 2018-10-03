
# ---------------------------------------------------------
#                     Examples Module
# ---------------------------------------------------------

message(STATUS "Building examples")

include_directories("${AX_SRC_ROOT}/src"
                    "${AX_SRC_ROOT}/audio" 
                    "${AX_SRC_ROOT}/DAO"
                    "${AX_SRC_ROOT}/tools")

# --- Check module-specific parameters ---

# ...

# --- Platform-specific module compilation settings ---

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

   set(APP_CXX_FLAGS "")
   
   set(THREAD_LIB pthread)
   
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
   
   set(APP_CXX_FLAGS "")
   
endif()

# --- Modules definitions ---

set(EXAMPLE1_SRCS example1.cpp 
                  ${AX_SRC_ROOT}/DAO/${DATASTORE_T}.cpp 
                  ${AX_SRC_ROOT}/audio/AudioSource.cpp)

set(EXAMPLE2_SRCS example2.cpp 
                  ${AX_SRC_ROOT}/DAO/${DATASTORE_T}.cpp)

set(EXAMPLE3_SRCS example3.cpp 
                  ${AX_SRC_ROOT}/DAO/${DATASTORE_T}.cpp 
                  ${AX_SRC_ROOT}/audio/AudioSource.cpp)

set(EXAMPLE4_SRCS example4.cpp 
                  ${AX_SRC_ROOT}/DAO/${DATASTORE_T}.cpp 
                  ${AX_SRC_ROOT}/audio/AudioSource.cpp)

# --- Check for required libraries ---

find_library(DATASTORE_LIB
             NAMES ${DATASTORE_LIB_NAME} 
             PATHS ${MY_LIBRARY_${ARCH}_${CMAKE_BUILD_TYPE}_DIRS})
			 
if(ID3_TAG_SUPPORT)
find_library(ID3TAG_LIB
             NAMES ${ID3TAG_LIB_NAME} 
             PATHS ${MY_LIBRARY_${ARCH}_${CMAKE_BUILD_TYPE}_DIRS})
endif()

# --- Build the modules ---

add_executable(example1 ${EXAMPLE1_SRCS})
target_compile_options(example1 PRIVATE "${APP_CXX_FLAGS}")
target_link_libraries(example1 audioneex
                      ${Boost_LIBRARIES} ${THREAD_LIB}
                      ${DATASTORE_LIB} ${ID3TAG_LIB})

add_executable(example2 ${EXAMPLE2_SRCS})
target_compile_options(example2 PRIVATE "${APP_CXX_FLAGS}")
target_link_libraries(example2 audioneex
                      ${Boost_LIBRARIES}
                      ${DATASTORE_LIB} ${ID3TAG_LIB})

add_executable(example3 ${EXAMPLE3_SRCS})
target_compile_options(example3 PRIVATE "${APP_CXX_FLAGS}")
target_link_libraries(example3 audioneex
                      ${Boost_LIBRARIES} ${THREAD_LIB}
                      ${DATASTORE_LIB} ${ID3TAG_LIB})

add_executable(example4 ${EXAMPLE4_SRCS})
target_compile_options(example4 PRIVATE "${APP_CXX_FLAGS}")
target_link_libraries(example4 audioneex
                      ${Boost_LIBRARIES} ${THREAD_LIB}
                      ${DATASTORE_LIB} ${ID3TAG_LIB})

