
![logo](https://www.audioneex.com/wp-content/uploads/2019/05/logo_280.png)

Audioneex is an audio content recognition engine specifically designed
for real-time applications. It is general purpose, based on content-agnostic
algorithms and runs on all kinds of machines, from big servers to mobile and 
embedded devices.

Features

- **Compact fingerprints** - 1 hr of audio encoded in less than a MB
- **Content-agnostic** - recognition of audio of different nature
- **Fast identification** - can be used for real-time recognitions
- **Cross-platform** - runs anywhere there is a modern C++ compiler (v11+)
- **IoT & Mobile-ready** - runs well on small devices for on-device ACR
- **Database-agnostic** - can use different databases by rewriting the drivers

For the more curious, it is an implementation of the methods described
in [this paper](https://www.dropbox.com/s/0qvfq2o53uudaqx/agramaglia_acr_paper_2014.pdf)


## Documentation

The official documentation can be found [here](https://audioneex.readthedocs.io)


## Quick start

Clone or download the repository into a directory of your choice. The engine 
needs the following dependencies

- Boost 1.6
- FFTSS 3.0
- Tokyo Cabinet 1.4 / Couchbase 5.1
- TagLib  (optional)
- FFMpeg  (optional)

After compiling and installing the dependencies, on Linux simply run the 
following command to build the engine

    $ cd <root_directory>
    $ mkdir build && cd build
    $ cmake ..
    $ make

On Windows, replace the last two commands above with the following

    > cmake -G "NMake Makefiles" [options] ..
    > nmake

The final libraries will be put into a `/lib` folder in the root directory.

Please refer to the [documentation](https://audioneex.readthedocs.io) for more details.


## Demo app

If you want to see the engine in action and play around with it straight away
there is a demo app for Windows that can be downloaded from [here](https://www.audioneex.com/downloads/)


## License

This code is released under the Mozilla Public License 2.0.

In a nutshell:

- You can use it in commercial projects without publishing your closed source code.
- If you distribute it under any form, you must retain all copyright notices 
  as they appear in the source files.
- If you distribute it in binary form, you must clearly state that your software 
  uses Audioneex and specify where its source code can be obtained.
- Any modifications to the source code must be made available, under the same license.

