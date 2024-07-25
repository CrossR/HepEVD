// global.hpp

#ifndef HEP_EVD_PY_GLOBAL_HPP
#define HEP_EVD_PY_GLOBAL_HPP

// Include the HepEVD header files.
#define HEP_EVD_BASE_HELPER 1
#include "hep_evd.h"

// STD includes
#include <tuple>
#include <map>

// Include nanobind
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

namespace HepEVD_py {

// Map from Python types to HepEVD types.
using RawHit = std::tuple<double, double, double, double>;
using PythonHitMap = std::map<RawHit, HepEVD::Hit *>;
inline PythonHitMap pythonHitMap;

void start_server(const int startState = -1, const bool clearOnShow = true);

} // namespace HepEVD_py

#endif // HEP_EVD_PY_GLOBAL_HPP