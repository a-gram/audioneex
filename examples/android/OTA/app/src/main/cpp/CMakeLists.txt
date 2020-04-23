
cmake_minimum_required(VERSION 3.6.0)

###########################################################
#                      User Config                        #
###########################################################

# Add here include paths specific to your environment
list(APPEND MY_INCLUDES
    "include"
    "/usr/local/include"
    "/usr/local/include/tcabinet"
    "/usr/local/share/boost_1_66_0"
)

###########################################################
#                      Build Config                       #
###########################################################

set(AX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../..")

set(ALL_INCLUDES
  "${AX_ROOT}/src"
  "${AX_ROOT}/src/audio"
  "${AX_ROOT}/src/dbdrivers"
   ${MY_INCLUDES})

# Some string manipulations to fix variations
string(REPLACE "." "" CLANG_VER "${CMAKE_CXX_COMPILER_VERSION}")
string(SUBSTRING "${CLANG_VER}" 0 4 CLANG_VER)
string(TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE)

# Create the JNI libs directory (used by Gradle for packaging)
set(LIB_DIR_TAG "android-${ANDROID_ABI}-clang${CLANG_VER}")
set(AX_LIB_DIR "${AX_ROOT}/lib/${LIB_DIR_TAG}/${BUILD_TYPE}")
set(JNI_LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}")

# Copy the native libs to the JNI libs dir
file(MAKE_DIRECTORY ${JNI_LIBS_DIR})
file(GLOB AX_LIBS ${AX_LIB_DIR}/*.so)
file(COPY ${AX_LIBS} DESTINATION ${JNI_LIBS_DIR})

add_library( audioneex SHARED IMPORTED )
set_target_properties( audioneex
                       PROPERTIES IMPORTED_LOCATION
                       "${AX_LIB_DIR}/libaudioneex.so" )

add_library( tokyocabinet SHARED IMPORTED )
set_target_properties( tokyocabinet
                       PROPERTIES IMPORTED_LOCATION
                       "${AX_LIB_DIR}/libtokyocabinet.so" )

add_library( tcDAO STATIC "${AX_ROOT}/src/dbdrivers/TCDataStore.cpp" )
target_include_directories(tcDAO PRIVATE ${ALL_INCLUDES})
target_compile_options(tcDAO PRIVATE -std=c++11)

find_library( llog log )

add_library( audioneex-jni SHARED "audioneex-jni.cpp" )
target_include_directories(audioneex-jni PRIVATE ${ALL_INCLUDES})
target_compile_options(audioneex-jni PRIVATE -std=c++11 -fexceptions)
target_link_libraries(audioneex-jni tcDAO tokyocabinet audioneex ${llog})

