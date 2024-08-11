// markers.hpp

#ifndef HEP_EVD_PY_MARKERS_HPP
#define HEP_EVD_PY_MARKERS_HPP

// Include nanobind headers
#include <nanobind/nanobind.h>
namespace nb = nanobind;

namespace HepEVD_py {

/**
 * Initializes the marker classes for the given module.
 *
 * @param m The module to initialize the marker classes for.
 */
void init_marker_classes(nb::module_ &m);

/**
 * Adds markers to the event display.
 *
 * @param markers The markers to add.
 */
void add_markers(nb::handle markers);

} // namespace HepEVD_py

#endif // HEP_EVD_PY_MARKERS_HPP