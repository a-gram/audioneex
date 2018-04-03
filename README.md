
Audioneex is an audio content recognition engine specifically designed
for real-time uses. It is general purpose, based on content-agnostic
algorithms and runs on desktop, server, mobile and embedded devices.

Features

- **Compact fingerprints** - 1 hr of audio encoded in less than a MB
- **Content-agnostic** - recognition of audio of different nature
- **Real-time identification** - can be used for Over-The-Air recognitions
- **Cross-platform** - runs anywhere there is a decent C++ compiler
- **IoT & Mobile-ready** - runs well on small devices for on-device ACR

Limitations

- Limited scaling on a single instance
- May take a while or fail on very distorted audio
- Documentation is (currently) not available

There is also a commercial version that provides more functionalities and
better performances at www.audioneex.com

For the more curious, it is a partial implementation of the methods described
in [this paper](https://www.dropbox.com/s/0qvfq2o53uudaqx/agramaglia_acr_paper_2014.pdf)


## Prerequisites

The code has been extensively tested with the following software

- Boost 1.66
- FFTSS 3.0
- CMake 3.11
- Android NDK r16b+

Optional (required by the examples)

- Tokyo Cabinet
- TagLib
- FFMpeg

However, it should also work with the following minimum requirements

- Boost 1.55+
- GCC 4.8+, VC 10+
- CMake 3.5+
- Android NDK 14b


## How to build on Linux and Windows

The project uses CMake to build the modules on both Linux and Windows.
The steps to follow are pretty much the same on both platforms, aside
for few specific tweaks that may occur.

**Step 1.**

Compile the [FFTSS library](http://www.ssisc.org/fftss/) in static
mode and install it somewhere in your system. You can set this path in
the CMake build script if it's not in a standard location. Remember
to compile the library with the `-fPIC` flag (on Linux) otherwise linking
errors will occur. If you want to build the examples as well, then you
will need to get the Tokyo Cabinet library (there is also a port to Windows
called 'tcejdb'). Alternatively, you can use the Couchbase database (requires
the libcouchbase driver). Optionally, should you need ID3 tag support to
extract metadata from audio files, then get the TagLib library.
Audioneex requires the header-only part of Boost, but the examples will
need some compiled modules (thread, filesystem), so you can just compile
those ones.

**Step 2.**

Set the include and library paths in the CMake build script. This step
is not mandatory but it will most likely be necessary since these paths
are system-dependent. You can do so in the "User Configuration" section
of the build script.

**Step 3.**

On Linux, open a shell and issue the following commands

    cd <source_root_directory>
    mkdir build && cd build
    cmake [options] ..
    make

where `[options]` is one or more of the following command line parameters in
the form `-D<var=value>`

    ARCH            = x32|x64
    TOOLCHAIN       = gcc64|vc14|...
    BINARY_TYPE     = dynamic|static
    BUILD_MODE      = debug|release
    WITH_EXAMPLES   = ON|OFF
    DATASTORE_T     = TCDataStore|CBDataStore  (for the examples only)
    ID3_TAG_SUPPORT = ON|OFF  (for the examples only)
    WITH_TESTS      = ON|OFF  (for project developers only)


On Windows, replace the last two commands above with the following

    cmake -G "NMake Makefiles" [options] ..
    nmake

By default, if no options are passed on, the script builds dynamic libraries
targeting 64-bit architectures and gcc 7.2 (vc 14.1 on Windows) release mode.
The final libraries will be put in the `/lib` folder of the root directory.


## How to build for Android

There is a build script in the root directory called `build_android` that
will ease the compilation of the library for Android platforms. This script
uses the Android NDK build system (ndk-build et al.) to build the library
without the need for exporting individual toolchains for each architecture.

Usage:

    build_android <arch> <comp> <api> <bmode> <btype>

where

    <arch>   is one of the supported architectures (armeabi-v7a, x86, etc.)
    <comp>   the compiler (clang, gcc)
    <api>    the target Android API version
    <bmode>  the build mode (debug, release)
    <btype>  the library type (static, dynamic)

The final libraries will be put in the `/lib` folder of the root directory.

As a prerequisite, you will have to build the required external libraries
for the specific Android platforms you're targeting. There is a build script
in the root directory called `android-configure` that will help you with the
cross-compilation of the libraries without the need for exporting toolchains
for each target architecture. For more info, have a look at the script itself.

This script has been tested with the NDK r16b. Please refer to the script for
more information (especially for how to fix some bugs present in r16b).


## License

This code is released under the Mozilla Public License 2.0.

In a nutshell:

- You can use it in commercial projects without publishing your closed source code
- If you distribute it publicly under any form (code or binary, stand-alone or linked) you 
  must make available its source code, including any modification you have made, under the 
  same license and retain all copyright notices.
  
  
## TODO

1. More drivers for other databases would be nice
2. A more comprehensive test suite
3. Use more C++ new features and drop old compilers support


