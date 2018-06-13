
project (audiocodes CXX)

set(CMAKE_CXX_FLAGS "${COMP_FLAGS} -fPIC")

# Modules

message(STATUS "[*] Preparing audiocodes build ...")

add_library(axcodes STATIC AudioCodes.cpp)


