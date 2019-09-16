
How to build it
===============

The code has been extensively tested on Linux, Windows and Android platforms,
so here we're outlining the steps needed to build the binaries on such
platforms. However, it may as well build on others not officially mentioned 
here. Generally, any POSIX-compliant platform with a modern C++ compiler
(version 11 or higher) should be fine.


Prerequisites
-------------

To build the full package the following dependencies are required

* Boost 1.6
* FFTSS 3.0
* Tokyo Cabinet 1.4 / Couchbase 5.1
* TagLib  (optional)
* FFMpeg  (optional)

and the following toolchains

* GCC 6/7, CLang 5, MSVC++ 14
* CMake 3.11
* Android NDK r16b

While the specified versions for the above dependencies are guaranteed to work 
and are thus recommended, it may also work with others. TagLib and FFMpeg are 
only needed for the example programs and can be replaced with something similar, 
but doing so will require changes to the code.


Database
--------

Audioneex is database-agnostic, so technically it can be used with any database. 
However, using databases other than the ones supported out of the box requires 
writing the drivers. The default databases are Tokyo Cabinet and Couchbase. 
The former is an embedded/in-process database (suitable for mobile/embedded apps), 
while the latter is a client/server type.


How to build for Linux and Windows
----------------------------------

The project uses the CMake build system on both Linux and Windows.
The steps to follow are pretty much the same on both platforms, aside
for few specific tweaks that may occur.

**1.  Set up the build environment**

* Install Boost. The library requires the header-only part, but the examples 
  will need some compiled modules (thread, filesystem, regex and their dependencies).
* Get the `FFTSS <http://www.ssisc.org/fftss/>`_ library, compile it in static
  mode and install it somewhere in your system (remember to compile with the
  ``-fPIC`` flag on Linux otherwise linking errors will occur).
* Get the `Tokyo Cabinet <https://fallabs.com/tokyocabinet/>`_ library (for 
  Windows there is a port from the EJDB project `here <https://github.com/Softmotions/ejdb/tree/ejdb_1.x>`_). 
  Alternatively, you can use the Couchbase database (requires building the *libcouchbase* 
  driver, which can be downloaded from their website).
* Get the `TagLib <https://taglib.org/>`_ library for ID3 tag support (used by 
  the examples to extract metadata from audio files).
* Get the `FFmpeg <https://ffmpeg.org/>`_ executable and install it in a location 
  visible from ``PATH`` (used by the examples to decode and read audio).

**2.  Set include and library paths**

This step is not mandatory but it will most likely be necessary since these paths
are system-dependent. You can set them in the "User Configuration" section
of the CMake build script located in the root directory.

**3.  Build**

On Linux, open a shell and issue the following commands

.. code-block:: bash

   $ cd <source_root_directory>
   $ mkdir build && cd build
   $ cmake [options] ..
   $ make

where ``[options]`` is one or more of the following command line parameters in
the form ``-D<var=value>``

.. code-block:: none

   ARCH          = x32|x64
   TOOLCHAIN     = gcc64|gcc72|vc14|...
   BINARY_TYPE   = dynamic|static
   BUILD_MODE    = debug|release
   DATASTORE_T   = TCDataStore|CBDataStore
   WITH_EXAMPLES = ON|OFF
   WITH_ID3      = ON|OFF  (for the examples only, includes metadata)
   WITH_TESTS    = ON|OFF  (for project developers only)

On Windows, replace the last two commands above with the following

.. code-block:: shell

   > cmake -G "NMake Makefiles" [options] ..
   > nmake

By default, if no options are passed on, the script builds dynamic libraries
targeting 64-bit architectures in release mode. The final libraries will be put 
in a ``/lib`` folder in the root directory.


How to build for Android
------------------------

There is a build script in the root directory called ``build_android`` that will 
facilitate the compilation of the library for Android platforms. This script
uses the Android NDK build system (ndk-build et al.), so it goes without saying
that the NDK must be properly installed prior to make any attempt to build.

.. note::

   The script has been tested with the NDK r16b. Please refer to the script 
   itself for more information (especially for how to fix some bugs present in r16b).

Usage:

.. code-block:: bash

   $ build_android <arch> <comp> <api> <bmode> <btype>

where

.. code-block:: none

   <arch>   is one of the supported architectures (armeabi-v7a, x86, etc.)
   <comp>   the compiler (clang, gcc)
   <api>    the target Android API version
   <bmode>  the build mode (debug, release)
   <btype>  the library type (static, dynamic)

The final libraries will be put in the ``/lib`` folder of the root directory.

Naturally, first you will have to build the required external libraries mentioned 
in the prerequisites for the specific Android platforms you're targeting. A build 
script in the root directory called ``android-configure`` will help you with the
cross-compilation of these libraries without the need for exporting toolchains
for each target architecture. For more info, have a look at the script itself.
Patched source code for the libraries that compile on Android can be downloaded
from `here <https://www.dropbox.com/s/kg9sn42d80lt0gt/audioneex_android_ext_libs.tar.gz>`_.
Just unpack them somewhere and run

.. code-block:: bash

   $ ./android-configure <arch> <comp> <api> [config_params]
   $ make
    
from within the respective directories, where ``<arch> <comp> <api>`` are the same 
as in the ``build_android`` script and ``[config_params]`` are library-specific
configuration parameters. Please have a look at the script itself for more details.

