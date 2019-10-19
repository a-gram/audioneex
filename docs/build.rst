
How to build it
===============

The code has been extensively tested on Linux, Windows and Android platforms,
so here we're outlining the steps needed to build the binaries on such
platforms. However, it may as well build on others not officially mentioned 
here. Generally, any platform with a decent POSIX compatibility and a modern C++ 
compiler (version 11 or higher) should be fine.


Prerequisites
-------------

The engine itself only needs the following dependencies to build and run

* Boost 1.6
* FFTSS 3.0
* Tokyo Cabinet 1.4 / Couchbase 5.1

To build the full package, including the examples, you need to add the following 
optional components

* TagLib
* FFmpeg

The code has been developed mostly using the below mentioned tools, but anything
more recent should also work fine, so these are considered the minimum
requirements

* GCC 6, CLang 5, MSC 19.1
* CMake 3.11
* Android NDK r19

While the specified versions for the above dependencies are guaranteed to work 
and are thus recommended, it may also work with others. TagLib and FFMpeg are 
only needed for the example programs and can be replaced with something similar, 
but doing so will require changes to the code.


About the database
------------------

Audioneex needs a database to store the fingerprints and is designed to be 
database-neutral, so technically it can be used with any database. 
However, using databases other than the ones supported out of the box requires 
writing the drivers. The default databases are *Tokyo Cabinet* and *Couchbase*. 
The former is an embedded/in-process database (suitable for mobile/embedded apps), 
while the latter is a client/server type.


Building steps
--------------

The project uses the CMake build system on all supported platforms.
The steps to follow are pretty much the same for all platforms, aside
for few specific tweaks that may occur.

**1.  Set up the build environment**

* Install Boost. The library requires the header-only part, but the examples 
  will need some compiled modules (thread, filesystem, regex and their dependencies).
* Get the `FFTSS <http://www.ssisc.org/fftss/>`_ library, compile it in static
  mode and install it somewhere in your system (remember to compile with the
  ``-fPIC`` flag on Linux otherwise linking errors will occur).
* Get the `Tokyo Cabinet <https://fallabs.com/tokyocabinet/>`_ library (for 
  Windows there is a port from the EJDB project `here <https://github.com/Softmotions/ejdb/tree/ejdb_1.x>`_). 
  Alternatively, you can use the Couchbase database, which has a nice and free
  "community edition" (requires building their *libcouchbase* driver).
* Get the `TagLib <https://taglib.org/>`_ library for ID3 tag support (used by 
  the examples to extract metadata from audio files).
* Get the `FFmpeg <https://ffmpeg.org/>`_ executable and install it in a location 
  visible from ``PATH`` (used by the examples to decode and read audio).

**2.  Set include and library paths**

This step is not mandatory but it will most likely be necessary since these paths
are system-dependent. You can set them in the *User Config* section of the main 
CMake build script (CMakeLists.txt) located in the root directory.

**3.  Build**

After cloning (or downloading) the code, open a shell and execute the following 
commands

.. code-block:: bash

   $ cd <source_root_directory>
   $ ./build [options]

where ``[options]`` is one or more of the following command line parameters in
the form ``variable=value``

.. code-block:: none

   TARGET        = linux | windows | android
   ARCH          = x32 | x64  (Linux/Windows)
                   armeabi-v7a | arm64-v8a | x86 | x86_64 (Android)
   API           = API version number (Android only)
   BINARY_TYPE   = dynamic | static
   BUILD_MODE    = debug | release
   DATASTORE_T   = TCDataStore | CBDataStore
   WITH_EXAMPLES = ON | OFF
   WITH_ID3      = ON | OFF  (for the examples only)
   WITH_TESTS    = ON | OFF  (for project developers only)

By default, if no options are passed on, the build targets Linux (or Windows) 
x64 and produces the dynamic library only, in release mode. The final libraries 
will be put in a ``/lib`` folder in the root directory.

To build for Android, specify the target as follows

.. code-block:: bash

   $ ./build TARGET=android [options]

If no other paramaters are given, then the build defaults to the armeabi-v7a
architecture and the latest API level. 

.. note::

   The parameters for the build script are case-sensitive.

Naturally, first you will have to build the required external libraries mentioned 
in the prerequisites for the specific Android platforms you're targeting. A build 
script in the root directory called ``android-configure`` will help you with the
cross-compilation of these libraries. For more info, refer to the script itself.
Patched source code for the libraries that compile on Android straight away can 
be downloaded from `here <https://www.dropbox.com/s/kg9sn42d80lt0gt/audioneex_android_ext_libs.tar.gz>`_.
Just unpack them somewhere and run

.. code-block:: bash

   $ ./android-configure <arch> <api> [config_params]
   $ make
    
from within the respective directories, where ``<arch> <api>`` are the same 
as in the ``build`` script and ``[config_params]`` are library-specific
configuration parameters. Please have a look at the script for more details.





