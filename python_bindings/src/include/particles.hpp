// particles.hpp

#ifndef HEP_EVD_PY_PARTICLES_HPP
#define HEP_EVD_PY_PARTICLES_HPP

// Standard includes
#include <string>

// Include nanobind headers
#include <nanobind/nanobind.h>
namespace nb = nanobind;

namespace HepEVD_py {

/**
 * Add the given list/array of particles to the server.
 *
 * @param particles The handle to the list/array of particles.
 * @param label The optional label for the particles (default: empty string).
 */
void add_particles(nb::handle particles, std::string label = "");

} // namespace HepEVD_py

#endif // HEP_EVD_PY_PARTICLES_HPP
