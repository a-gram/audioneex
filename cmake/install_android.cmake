#
# Android binaries installation script
# ------------------------------------
#
# The installation for Android binaries is basically the same as for
# Linux, with the only difference that the STL library is also included
# in the installation dir as it will be packaged by the NDK.
#
include(../cmake/install_linux.cmake)

file(COPY ${AX_ANDROID_LIBC} DESTINATION ${AX_INSTALL_LIB_DIR})

