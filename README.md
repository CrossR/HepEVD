# HepEVD - <a href="https://crossr.github.io/HepEVD/" alt="Contributors"><img src="https://img.shields.io/badge/Live_Demo-blue" /></a>

A header-only web-based event display for particle physics events.

![image](https://github.com/CrossR/hep_evd/assets/10038688/badd2e8d-9a88-492f-8f1e-b41094af7e72)

## Code Layout

The main codebase is split into two sections: the C++ code, and the Javascript code.
There is then two further folders, containing the example code, and the Python bindings.

- `include` contains all the actual C++ library code that is responsible for storing
  the basic objects needed (hits, detector layouts, event markers), as well as the HTTP
  server that then serves these objects up for the WebUI to show. A HTTP server (serving
  a JSON API and the UI frontend) was chosen as it works nicely on remote systems, and
  allows quick changes to be made to the UI without re-running anything.

- `web` contains the actual Javascript code for the event display, utilising `THREE.js`
  heavily.

- `python_bindings` contains the Python bindings for the C++ code, allowing the C++
  library to be used from Python.

- `example` contains example code, showing how to use the library. This includes
  a simple example, showing randomly placed hits in a DUNE FD sized module, as
  well as an optional client/server example, which demonstrates how to use the
  library in a client/server mode, which can be useful to preserve state between
  events. Finally, there is an example of how the Python bindings can be used.

## Installation + Usage

To run the basic example, you need to pull down the external dependencies, then simply
build the example.

```
./get_extern_deps.sh
cd example/
make basic
./basic
```

On remote machines, you should be able to use port forwarding to access the webserver
that the example sets up from your local browser.

Alternatively, to build and then install the Python bindings, you can run:

```
./get_extern_deps.sh

# We use a git submodule for nanobind, the Python / C++ bindings library.
git submodule update --init --recursive

# You may want to setup a Python Venv for this first...
# https://docs.python.org/3/library/venv.html
python -m venv .venv
source .venv/bin/activate

# Swap to the Python bindings directory.
cd python_bindings/

# Build and install the bindings...
pip install .

# Test them...
python

$ import HepEVD
```

From there, you can then use the library as normal.

An example of how the library works can be seen in
`example/test_python_bindings.py`, as well as in the HepEVD wiki.

## Project Integration

There is some basic support for pulling in HepEVD into a CMake-based project.
This is usually not really necessary for basic debugging, but could be useful if
HepEVD is used extensively.

```cmake
include(FetchContent)

FetchContent_Declare(
    HepEVD
    GIT_REPOSITORY https://github.com/CrossR/hep_evd.git
    GIT_BRANCH main
)

FetchContent_MakeAvailable(HepEVD)

# Link the library to a target
target_link_libraries(MyBigProject PRIVATE HepEVD)
```

## Motivation

It is often useful to be able to view how the interactions in a particle physics event
look, to get a better understanding of the event topology and how reconstruction
algorithms are operating.

However, most built-in or available event displays have two main issues:

- Lack of availability: In a lot of cases, you can not easily spin up an event display
  whenever you want, meaning that if what you want to look at is not an input or an
  output, the raw hits or the final reconstruction, you are out of luck. This is not
  always the case however, but still, general availability of "I want an event display
  here, to show this thing" was a large motivation.

- A secondary, and arguably even bigger limitation is the sorts of available event
  displays. You may be able to spin up an event display wherever you want in some
  cases, but they are limited by C++ based GUIs, restricting you to basic figures and
  requiring hacks and dodgy workarounds to show the actual information you want, rather
  than utilising more modern and easily hackable interfaces, such as those provided in
  the web browser.

The final goal of this library aims to fix those two issues: a simple, header-only
include that can be dropped in without changes to a build system or more, and allows
events to be easily shown in the browser. Using the browser means modern, flexible 3D
drawing tools can be used, not ROOT-based or similar GUIs, with a further advantage that
a web-based event display trivially works remotely, if you use SSH forwarding.

## Acknowledgements

The HTTP server in this project utilises
[cpp-httplib](https://github.com/yhirose/cpp-httplib), to make the server code as simple
as possible. Similarly, both [nlohmann/json](https://github.com/nlohmann/json) and
[RapidJSON](https://github.com/Tencent/rapidjson) are used to both
convert and parse objects to/from JSON, such that they can be served via the HTTP server.

## Licensing

This code is released under the MIT License, though there are some exceptions.
All of the bundled code has its own license, and there is additional license constraints
on the code in `./integrations/`, due to the nature of that code building upon existing
files from other libraries. Where appropriate, the license has been noted and included
at the top of each file.

All of the downloaded external dependencies are MIT-licenesed, and their specific
licenses can be found here:

 - https://github.com/yhirose/cpp-httplib/blob/master/LICENSE
 - https://github.com/nlohmann/json/blob/develop/LICENSE.MIT
 - https://github.com/Tencent/rapidjson/blob/master/license.txt
