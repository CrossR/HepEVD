//
// HepEVD Python Bindings
//

#ifndef HEP_EVD_PYTHON_H
#define HEP_EVD_PYTHON_H

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>

// Include the HepEVD header files.
#define HEP_EVD_BASE_HELPER 1
#include "hep_evd.h"

// And any local includes...
#include "include/geometry.hpp"
#include "include/global.hpp"
#include "include/hits.hpp"
#include "include/markers.hpp"

namespace nb = nanobind;

/**
 * Loads the HepEVD configuration from a provided Python dictionary.
 *
 * @param config A Python dictionary containing the configuration settings.
 *
 * @throws std::runtime_error If the provided config is not a dictionary.
 */
void load_config(nb::object config) {

    if (!nb::isinstance<nb::dict>(config))
        throw std::runtime_error("HepEVD: Config must be a dictionary");

    nb::dict config_dict = nb::cast<nb::dict>(config);

    for (auto keyValueHandle : config_dict.items()) {
        std::string key = nb::cast<std::string>(keyValueHandle[0]);
        std::string value = nb::cast<std::string>(keyValueHandle[1]);
        HepEVD::hepEVDServer->getConfig()->set(key, value);
    }
}

/**
 * Set what the current interaction string is.
 *
 * @param mc_string A Python string containing the MC interaction string.
 */
void set_mc_string(nb::object mc_string) {

    if (!nb::isinstance<nb::str>(mc_string))
        throw std::runtime_error("HepEVD: MC string must be a string");

    HepEVD::hepEVDServer->setMCTruth(nb::cast<std::string>(mc_string));
}

NB_MODULE(_hepevd_impl, m) {

    m.doc() = "HepEVD - High Energy Physics Event Display";

    m.def("is_initialised", &HepEVD::isServerInitialised,
          "Checks if the server is initialised - i.e. does a server exists, with the geometry set?",
          nb::arg("quiet") = false);
    m.def("start_server", &HepEVD_py::start_server, "Starts the HepEVD server", nb::arg("start_state") = -1,
          nb::arg("clear_on_show") = true);
    m.def("set_verbose", &HepEVD::setVerboseLogging, "Sets the verbosity of the HepEVD server", nb::arg("verbose"));

    m.def("save_state", &HepEVD::saveState, "Saves the current state", nb::arg("state_name"), nb::arg("min_size") = -1,
          nb::arg("clear_on_show") = true);
    m.def("reset_server", &HepEVD::resetServer, "Resets the server", nb::arg("reset_geo") = false);

    m.def("set_mc_string", &set_mc_string, "Sets the current MC interaction string", nb::arg("mc_string"));
    m.def("set_config", &load_config,
          "Sets any top level config options for the server.\n"
          "This can include the following:\n"
          "  - show2D (default: 1)\n"
          "  - show3D (default: 1)\n"
          "  - disableMouseOver (default: 0)\n"
          "  - hitColour (default: 'grey')\n"
          "  - hitSize (default: Varies for 2D vs 3D)\n"
          "  - hitTransparency (default: 1.0)\n",
          nb::arg("config"), nb::sig("def set_config(config: dict[str, str]) -> None"));

    // Set the current HepEVD geometry.
    // Input will either be a string or a list/array of numbers.
    m.def("set_geometry", &HepEVD_py::set_geometry, "Sets the geometry of the server", nb::arg("geometry"),
          nb::sig("def set_geometry(geometry: typing.Union[str, "
                  "collections.abc.Collection[collections.abc.Collection[float | int]]]) -> None"));

    m.def("add_hits", &HepEVD_py::add_hits<HepEVD::Hit>,
          "Adds hits to the current event state.\n"
          "Hits must be passed as an (NHits, Y) list or array, with the columns being "
          "(x, y, z, energy) and two optional columns (view, dimension) for the hit type and dimension.\n"
          "The view and dimension values must be from the HepEVD.HitType and HepEVD.HitDimension enums respectively.",
          nb::arg("hits"), nb::arg("label") = "",
          nb::sig("def add_hits(hits: collections.abc.Collection[collections.abc.Collection[float | int | HitType | "
                  "HitDimension]], "
                  "label: str = '') -> None"));
    m.def("add_mc", &HepEVD_py::add_hits<HepEVD::MCHit>,
          "Adds MC hits to the current event state.\n"
          "Hits must be passed as an (NHits, Y) list or array, with the columns being "
          "(x, y, z, energy, PDG) and two optional columns (view, dimension) for the hit type and dimension.\n"
          "The view and dimension values must be from the HepEVD.HitType and HepEVD.HitDimension enums respectively.",
          nb::arg("mcHits"), nb::arg("label") = "",
          nb::sig("def add_mc(hits: collections.abc.Collection[collections.abc.Collection[float | int | HitType | "
                  "HitDimension]], "
                  "label: str = '') -> None"));
    m.def("add_hit_properties", &HepEVD_py::set_hit_properties,
          "Add custom properties to a hit, via a string / double dictionary.\n"
          "The hit must be passed as a (x, y, z, energy) list or array.",
          nb::arg("hit"), nb::arg("properties"),
          nb::sig("def add_hit_properties(hit: collections.abc.Collection[float | int], properties: "
                  "typing.Dict[float | int]) "
                  "-> None"));
    m.def("add_markers", &HepEVD_py::add_markers,
          "Adds markers to the current event state.\n"
          "Markers must be passed as a list or array of marker objects."
          "The various marker types are Point, Line and Ring."
          "Any required parameters (labels, colours, hit dims etc), should be applied to the underlying object.",
          nb::arg("markers"), nb::sig("def add_markers(markers: collections.abc.Collection[HepEVD.Marker]) -> None"));

    // Add enums
    nb::enum_<HepEVD::HitType>(m, "HitType",
                               "Enum for various possible hit types. This is mostly useful for LArTPC view data.",
                               nb::is_arithmetic())
        .value("GENERAL", HepEVD::HitType::GENERAL, "General hit type")
        .value("TWO_D_U", HepEVD::HitType::TWO_D_U, "A 2D U View hit, from a LArTPC")
        .value("TWO_D_V", HepEVD::HitType::TWO_D_V, "A 2D V View hit, from a LArTPC")
        .value("TWO_D_W", HepEVD::HitType::TWO_D_W, "A 2D W View hit, from a LArTPC");

    nb::enum_<HepEVD::HitDimension>(m, "HitDimension", "Enum to distinguish between 3D and 2D hits.",
                                    nb::is_arithmetic())
        .value("TWO_D", HepEVD::HitDimension::TWO_D, "A 2D hit")
        .value("THREE_D", HepEVD::HitDimension::THREE_D, "A 3D hit");

    // Add marker classes
    HepEVD_py::init_marker_classes(m);
}

#endif // HEP_EVD_PYTHON_H