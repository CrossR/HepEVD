# HEP EVD

A header-only web-based event display for particle physics events.

## Code Layout

The codebase is split into two sections: the C++ code, and the Javascript code:

- `include` contains all the actual C++ library code that is responsible for storing
  the basic objects needed (hits, detector layouts, event markers), as well as the HTTP
  server that then serves these objects up for the WebUI to show. A HTTP server (serving
  a JSON API and the UI frontend) was chosen as it works nicely on remote systems, and
  allows quick changes to be made to the UI without re-running anything.

- `web` contains the actual Javascript code for the event display, utilising `THREE.js`
  heavily.

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
as possible. Similarly, [nlohmann/json](https://github.com/nlohmann/json) is used to both
convert and parse objects to/from JSON, such that they can be served via the HTTP server.

## Future Work

- More helper functions.

- How easy / how expensive is adding a hoverover action?

- Hit widths? Pain with instanced mesh though.

- Fill out URL data loading: User configurable, loading bar.