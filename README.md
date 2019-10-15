
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


## Documentation

The official documentation can be found [here](https://audioneex.readthedocs.io)


## Quick start

The engine needs the following dependencies

- Boost
- FFTSS
- Tokyo Cabinet | Couchbase
- TagLib  (optional)
- FFmpeg  (optional)

After compiling and installing the dependencies, open a shell and issue the 
following commands to start the build process

    $ git clone https://github.com/a-gram/audioneex.git
    $ cd audioneex
    $ ./build

The final libraries will be put into a `/lib` folder in the root directory.

Please refer to the [documentation](https://audioneex.readthedocs.io) for more 
details.


## Demo app

If you want to see the engine in action and play around with it straight away
there is a demo app for Windows that can be downloaded from [here](https://www.audioneex.com/downloads/)


## License

This code is released under the Mozilla Public License 2.0.

In a nutshell:

- It can be used freely in commercial projects without publishing proprietary code.
- Any modifications to the source code must be made available, under the same license.
- If distributed under any form (source or binary) all copyright notices must be 
  retained (for binaries, the notices can be put in the docs or "about" box).


## References

For the more curious, it is an implementation of the methods described
in [this paper](https://www.dropbox.com/s/0qvfq2o53uudaqx/agramaglia_acr_paper_2014.pdf)
