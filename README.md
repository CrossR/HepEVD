# HEP EVD

A header-only web-based event display for particle physics events.

## Installation + Usage

TODO

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
as possible.

## Future Work

 - Can / should we compile to a truly single-header solution, bundling in HTTPLib,
   assuming the license allows it? (Its MIT so I'd guess so). Could likely have that as
   a build step on GitHub Actions.
 - Helper functions for Pandora, LArSoft + more to convert bits.

