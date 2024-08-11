# HepEVD - <a href="https://crossr.github.io/HepEVD/" alt="Contributors"><img src="https://img.shields.io/badge/Live_Demo-blue" /></a>

A header-only web-based event display for particle physics events.

![image](https://github.com/CrossR/hep_evd/assets/10038688/badd2e8d-9a88-492f-8f1e-b41094af7e72)

Look at https://github.com/CrossR/HepEVD/ for more technical details around the underlying C++
library used here, as well as the Javascript code that powers the event display itself.

## Installation + Usage

To get the python-bindings for HepEVD, either install the library as a normal
python package (`pip install HepEVD`), or follow the instructions below once
fully cloned, to build from source:

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

## TODO

 - Add ability to pass over clusters / particles:
    - I.e. groups of hits, and hierarchies of groups of hits
    - Need to work out the best interface for that...
