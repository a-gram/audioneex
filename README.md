
![logo](https://www.audioneex.com/wp-content/uploads/2019/05/logo_280.png)

Audioneex-OS is an audio content recognition engine specifically designed
for real-time applications. It is general purpose, based on content-agnostic
algorithms and runs on all kinds of machines, from big servers to mobile and 
embedded devices.

Features

- **Compact fingerprints** - 1 hr of audio encoded in less than a MB
- **Content-agnostic** - recognition of audio of different nature
- **Fast identification** - can be used for real-time recognitions
- **Cross-platform** - runs anywhere there is a fully compliant C++11 compiler
- **IoT & Mobile-ready** - runs well on small devices for on-device ACR
- **Database-agnostic** - can use different databases by rewriting the drivers

Limitations

- Limited scaling on a single instance
- Limited robustness to noisy over-the-air audio
- Documentation is (currently) not available

A commercial version that provides more functionalities and better performances 
is available at www.audioneex.com

For the more curious, it is a partial implementation of the methods described
in [this paper](https://www.dropbox.com/s/0qvfq2o53uudaqx/agramaglia_acr_paper_2014.pdf)


## Prerequisites

The engine needs, and has been tested with, the following dependencies

- Boost 1.6
- FFTSS 3.0
- Tokyo Cabinet 1.4 / Couchbase 5.1
- TagLib  (optional)
- FFMpeg  (optional)

and the following tools

- GCC 6/7, CLang 5, MSVC++ 14
- CMake 3.11
- Android NDK r16b

However, it should also work with any other fully compliant C++11 compiler.
The TagLib and FFMpeg can be replaced with something else but you will need
to make changes to the code.

## Database

Audioneex is database-agnostic, so technically it can be used with any database. 
However, using databases other than the default ones requires writing the drivers 
by implementing the exposed interfaces and following the specifications. The default
databases are Tokyo Cabinet and Couchbase. The former is an embedded/in-process
database (suitable for mobile/embedded apps), while the latter is client/server.


## How to build on Linux and Windows

The project uses the CMake build system on both Linux and Windows.
The steps to follow are pretty much the same on both platforms, aside
for few specific tweaks that may occur.

**Step 1. Set up the build environment**

- Install Boost. The library requires the header-only part, but the examples 
will need some compiled modules (thread, filesystem, regex and their dependencies).
- Get the [FFTSS](http://www.ssisc.org/fftss/) library, compile it in static
mode and install it somewhere in your system (remember to compile with the
`-fPIC` flag on Linux otherwise linking errors will occur).
- Get the [Tokyo Cabinet](https://fallabs.com/tokyocabinet/) library (for Windows 
there is a port from the EJDB project [here](https://github.com/Softmotions/ejdb/tree/ejdb_1.x) ). 
Alternatively, you can use the Couchbase database (requires building the libcouchbase 
driver, which can be downloaded from the website).
- Get the [TagLib](https://taglib.org/) library for ID3 tag support (used by 
the examples to extract metadata from audio files).
- Get the [FFmpeg](https://ffmpeg.org/) executable and install it in a location 
visible from `PATH` (used by the examples to decode and read audio).

**Step 2. Set include and library paths**

This step is not mandatory but it will most likely be necessary since these paths
are system-dependent. You can set them in the "User Configuration" section
of the CMake build script located in the root directory.

**Step 3. Build**

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
targeting 64-bit architectures release mode. The final libraries will be put 
in a `/lib` folder in the root directory.


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

- You can use it in commercial projects without publishing your closed source code.
- If you distribute it under any form or modify it, you must make its source code 
  available, including your modifications, under the same license and retain all 
  copyright notices.
  
  
## TODO

1. More drivers for other databases would be nice
2. A more comprehensive test suite
3. Use more C++ new features and drop old compilers support


