// geometry.hpp

#ifndef HEP_EVD_PY_GEOMETRY_HPP
#define HEP_EVD_PY_GEOMETRY_HPP

#include <nanobind/nanobind.h>
namespace nb = nanobind;

namespace HepEVD_py {

void set_geometry(nb::object geometry);

} // namespace HepEVD_py

#endif // HEP_EVD_PY_GEOMETRY_HPP