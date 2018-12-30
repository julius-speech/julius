# libfvad: voice activity detection (VAD) library #
[![Build Status](https://travis-ci.org/dpirch/libfvad.svg?branch=master)](https://travis-ci.org/dpirch/libfvad)

This is a fork of the VAD engine that is part of the WebRTC Native Code package
(https://webrtc.org/native-code/), for use as a standalone library independent
from the rest of the WebRTC code. There are currently no changes in
functionality.

## Building and Installing ##
libfvad uses autoconf/automake and can be build and installed with the usual:
```
./configure
make
sudo make install
```

 - When building from the cloned git repository (instead of a downloaded release),
   run `autoreconf -i` to create the missing *configure* script.
 - An optional example can be enabled enabled by `./configure --enable-examples`.
   This requires libsndfile (http://www.mega-nerd.com/libsndfile/, e.g.
   `apt install libsndfile1-dev`).

## Usage ##
The API is documented in the `include/fvad.h` header file. See also
`examples/fvadwav.h`.

## Development notes ##
Recommended CFLAGS to turn on warnings: `-std=c11 -Wall -Wextra -Wpedantic`.
Tests can be run with `make check`.

### Origin ###
This library largely consists of parts of the WebRTC Native Code package, the
repository of which can be found at
https://chromium.googlesource.com/external/webrtc:

 - Most of `webrtc/common_audio/vad/` has been moved to `src/vad`.
 - Parts of `webrtc/common_audio/signal_processing` have been moved to
   `src/signal_processing`. Parts of this signal processing library not needed
   by the VAD engine have been removed. Also, some platform-specific assembly
   code has been removed for now, for easier maintainability.
 - Relevant unit tests have been converted into automake tests and moved to
   `tests`.

### Merging upstream changes ###
It is intended that future changes and fixes in the WebRTC Native Code package
will also be be merged into libfvad.

To help with this, the libfvad git
repository has an `upstream-import` branch containing the required subset of the
WebRTC Native Code package's files, and an `upstream-renamed` branch which also
contains these unmodified files, but moved/renamed to the libfvad directory
structure.

The `tools/import.sh` script is intended to be run in the
`upstream-import` branch and imports changes from a local clone of the WebRTC
Native Code package git repository; it reads `tools/import-paths` which contains
the list of files to import, and reads and updates `tools/import-commit` which
contains the most recent imported commit hash of the source repository.

After this import, the changes can be merged first into the `upstream-renamed`
branch and then into the `master` branch. The intermediate step is necessary
because *git merge* would treat files that were both renamed and heavily changed
as new files.
