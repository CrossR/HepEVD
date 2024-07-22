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
    std::vector<T*> hepEVDHits;

    for (int i = 0; i < rows; i++) {
        double x = data[i * cols + 0];
        double y = data[i * cols + 1];
        double z = data[i * cols + 2];
        double energy = data[i * cols + 3];

       T *hit = new T(HepEVD::Position({x, y, z}), energy);
       
        if constexpr (std::is_same<T, HepEVD::MCHit>::value)
            hit->setPDG(data[i * cols + 4]);

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
          py::arg("hits"), py::arg("label") = "");
}