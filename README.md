
![logo](https://www.audioneex.com/wp-content/uploads/2019/05/logo_280.png)

[![Build Status](https://travis-ci.org/a-gram/audioneex.svg?branch=master)](https://travis-ci.org/a-gram/audioneex)
[![Documentation Status](https://readthedocs.org/projects/audioneex/badge/?version=latest)](https://audioneex.readthedocs.io/en/latest/?badge=latest)
[![License: MPL 2.0](https://img.shields.io/badge/License-MPL%202.0-brightgreen.svg)](https://opensource.org/licenses/MPL-2.0)


### *** _THIS IS AN UNSTABLE BRANCH! PLEASE USE THE MASTER BRANCH._ ***

Audioneex is an audio content recognition engine with audio fingerprinting
technology specifically designed for real-time applications. It is general 
purpose, based on content-agnostic algorithms and runs on all kinds of machines.

Features

- **Compact fingerprints** - 1 hr of audio encoded in less than 1 MB
- **Fast identification** - only a few seconds to perform a recognition
- **Content-agnostic** - recognition of audio of different nature
- **Cross-platform** - runs anywhere there is a modern C++ compiler (v11+)
- **IoT & Mobile-ready** - runs well on small devices for on-device ACR
- **Database-neutral** - can be used with any database (requires drivers)


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
    $ ./build WITH_EXAMPLES=ON

This will build the library along with the demo programs within the `_build` directory. 
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
in [this paper](https://www.dropbox.com/s/0qvfq2o53uudaqx/agramaglia_acr_paper_2014.pdf).
If you are including this work in your research, please use the following BibTex citation

    @misc{agramaglia2014-acr,
       author =       "Alberto Gramaglia",
       title =        "A Binary Auditory Words Model for Audio Content Identification",
       howpublished = "\url{https://github.com/a-gram/audioneex}",
       year =         "2014"
    }

or in plain text

_A. Gramaglia, "A Binary Auditory Words Model for Audio Content Identification", Audioneex.com, 2014._
