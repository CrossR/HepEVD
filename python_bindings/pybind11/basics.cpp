#include <pybind11/pybind11.h>

#include <cstdlib>
#include <map>
#include <signal.h>

// Include the HepEVD header files.
#define HEP_EVD_BASE_HELPER 1
#include "hep_evd.h"

// And any local includes...
#include "../include/detectors.hpp"

namespace py = pybind11;

// Map from Python types to HepEVD types.
using RawHit = std::tuple<double, double, double, double>;
using PythonHitMap = std::map<RawHit, HepEVD::Hit *>;
inline PythonHitMap pythonHitMap;

// Set the current HepEVD geometry.
// Input will either be a string or a list/array of numbers.
void set_geometry(py::object geometry) {

    if (HepEVD::isServerInitialised())
        return;

    HepEVD::Volumes volumes;

    if (py::isinstance<py::str>(geometry)) {
        if (detectors.find(geometry.cast<std::string>()) == detectors.end())
            throw std::runtime_error("Unknown detector: " + geometry.cast<std::string>());

        volumes = detectors[geometry.cast<std::string>()];
    } else if (py::isinstance<py::buffer>(geometry)) {

        py::buffer_info info = geometry.cast<py::buffer>().request();

        if (info.shape[1] != 6)
            throw std::runtime_error("Geometry array must have 6 columns");

        double *data = static_cast<double *>(info.ptr);

        for (int i = 0; i < info.shape[0]; i++) {
            volumes.push_back(BoxVolume(Position({data[i * 6 + 0], data[i * 6 + 1], data[i * 6 + 2]}), data[i * 6 + 3],
                                        data[i * 6 + 4], data[i * 6 + 5]));
        }
    } else {
        throw std::runtime_error("Unknown geometry type, must be string or array");
    }

    hepEVDServer = new HepEVDServer(DetectorGeometry(volumes));

    // Register the clear function for the hit map,
    // so we can clear it when we need to.
    HepEVD::registerClearFunction([&]() { pythonHitMap.clear(); });
}

// Add the given list/array of hits to the server.
// Works for either HepEVD::Hit or HepEVD::MCHit.
template <typename T> void add_hits(py::buffer hits, std::string label = "") {

    if (!HepEVD::isServerInitialised())
        return;

    py::buffer_info hits_info = hits.request();

    if (hits_info.ndim != 2)
        throw std::runtime_error("Hits array must be 2D");

    // Check that the shape of the array is correct.
    // We are expecting the shape to be (N, 4) where N is the number of hits.
    int expectedSize = std::is_same<T, HepEVD::Hit>::value ? 4 : 5;
    if (hits_info.shape[1] != expectedSize)
        throw std::runtime_error("Hits array must have " + std::to_string(expectedSize) + " columns");

    double *data = static_cast<double *>(hits_info.ptr);
    int rows = hits_info.shape[0];
    int cols = hits_info.shape[1];

    // Process all the hits in the array.
    std::vector<T *> hepEVDHits;

    for (int i = 0; i < rows; i++) {

        double x = data[i * cols + 0];
        double y = data[i * cols + 1];
        double z = data[i * cols + 2];
        double energy = data[i * cols + 3];

        T *hit = new T(HepEVD::Position({x, y, z}), energy);

        if constexpr (std::is_same<T, HepEVD::MCHit>::value) {
            hit->setPDG(data[i * cols + 4]);
        } else {
            pythonHitMap[std::make_tuple(x, y, z, energy)] = hit;
        }

        if (label != "")
            hit->setLabel(label);

        hepEVDHits.push_back(hit);
    }

    if constexpr (std::is_same<T, HepEVD::MCHit>::value)
        HepEVD::getServer()->addMCHits(hepEVDHits);
    else
        HepEVD::getServer()->addHits(hepEVDHits);
}

PYBIND11_MODULE(hepevd, m) {

    m.doc() = "HepEVD - High Energy Physics Event Display";

    m.def("start_server", &HepEVD::startServer, "Starts the HepEVD server", py::arg("start_state") = -1,
          py::arg("clear_on_show") = true);
    m.def("is_server_initialised", &HepEVD::isServerInitialised,
          "Checks if the server is initialised - i.e. is the geometry set?");
    m.def("set_verbose", &HepEVD::setVerboseLogging, "Sets the verbosity of the HepEVD server", py::arg("verbose"));

    m.def("save_state", &HepEVD::saveState, "Saves the current state", py::arg("state_name"), py::arg("min_size") = -1,
          py::arg("clear_on_show") = true);
    m.def("reset_server", &HepEVD::resetServer, "Resets the server", py::arg("reset_geo") = false);

    m.def("add_hits", &add_hits<HepEVD::Hit>,
          "Adds hits to the current event state. Hits must be passed an (N, 4) list or array, with the 4 columns being "
          "(x, y, z, energy)",
          py::arg("hits"), py::arg("label") = "");
    m.def("add_mc", &add_hits<HepEVD::MCHit>,
          "Adds hits to the current event state. Hits must be passed an (N, 5) list or array, with the 5 columns being "
          "(x, y, z, energy, PDG)",
          py::arg("mcHits"), py::arg("label") = "");
}