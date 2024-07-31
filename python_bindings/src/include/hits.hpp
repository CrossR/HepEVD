// hits.hpp

#ifndef HEP_EVD_PY_HITS_HPP
#define HEP_EVD_PY_HITS_HPP

// Standard includes
#include <string>

// Include nanobind headers
#include <nanobind/nanobind.h>
namespace nb = nanobind;

namespace HepEVD_py {

/**
 * Add the given list/array of hits to the server.
 *
 * @param hits The handle to the list/array of hits.
 * @param label The optional label for the hits (default: empty string).
 */
template <typename T> void add_hits(nb::handle hits, std::string label = "");

/**
 * Apply properties to the given hit.
 *
 * @param hit The handle to the hit.
 * @param properties The dictionary of properties to be applied.
 */
void set_hit_properties(nb::handle hit, nb::dict properties);

} // namespace HepEVD_py

#endif // HEP_EVD_PY_HITS_HPP