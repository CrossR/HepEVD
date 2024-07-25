// hits.hpp

#ifndef HEP_EVD_PY_HITS_HPP
#define HEP_EVD_PY_HITS_HPP

// Standard includes
#include <string>

// Include nanobind headers
#include <nanobind/nanobind.h>
namespace nb = nanobind;

namespace HepEVD_py {

    // Add the given list/array of hits to the server.
    // Works for either HepEVD::Hit or HepEVD::MCHit.
    template <typename T> void add_hits(nb::handle hits, std::string label = "");

    // Apply properties to the given hit.
    void set_hit_properties(nb::handle hit, nb::dict properties);
}

#endif // HEP_EVD_PY_HITS_HPP