#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>

#include <cstdlib>
#include <map>
#include <signal.h>

// Include the HepEVD header files.
#define HEP_EVD_BASE_HELPER 1
#include "hep_evd.h"

// And any local includes...
#include "../include/detectors.hpp"

namespace nb = nanobind;

// Map from Python types to HepEVD types.
using RawHit = std::tuple<double, double, double, double>;
using PythonHitMap = std::map<RawHit, HepEVD::Hit *>;
inline PythonHitMap pythonHitMap;

bool isArrayOrList(nb::handle obj) { return nb::isinstance<nb::ndarray<>>(obj) || nb::isinstance<nb::list>(obj); }

// Set the current HepEVD geometry.
// Input will either be a string or a list/array of numbers.
void set_geometry(nb::object geometry) {

    if (HepEVD::isServerInitialised())
        return;

    HepEVD::Volumes volumes;

    // If the input is a string, check if it is a detector.
    // Otherwise if an array or list, assume it is a list of volumes.
    if (nb::isinstance<nb::str>(geometry)) {
        if (detectors.find(nb::cast<std::string>(geometry)) == detectors.end())
            throw std::runtime_error("Unknown detector: " + nb::cast<std::string>(geometry));

        volumes = detectors[nb::cast<std::string>(geometry)];
    } else if (isArrayOrList(geometry)) {

        nb::ndarray<> array = nb::cast<nb::ndarray<>>(geometry);

        // TODO: Extend this to other geometry types.
        if (array.shape_ptr()[1] != 6)
            throw std::runtime_error("Geometry array must have 6 columns");

        double *data = static_cast<double *>(array.data());

        for (int i = 0; i < array.shape_ptr()[0]; i++) {
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
template <typename T> void add_hits(nb::ndarray<> hits, std::string label = "") {

    if (!HepEVD::isServerInitialised())
        return;

    if (hits.ndim() != 2)
        throw std::runtime_error("Hits array must be 2D");

    // Check that the shape of the array is correct.
    // We are expecting the shape to be (N, 4) where N is the number of hits.
    int expectedSize = std::is_same<T, HepEVD::Hit>::value ? 4 : 5;
    if (hits.shape_ptr()[1] != expectedSize)
        throw std::runtime_error("Hits array must have " + std::to_string(expectedSize) + " columns");

    double *data = static_cast<double *>(hits.data());
    int rows = hits.shape_ptr()[0];
    int cols = hits.shape_ptr()[1];

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

// Apply properties to the given hit or hits.
void set_hit_properties(nb::ndarray<> hit, nb::dict properties) {

    if (!HepEVD::isServerInitialised())
        return;

    if (hit.ndim() != 1)
        throw std::runtime_error("Hit array must be 1D");
    else if (hit.shape_ptr()[0] != 4)
        throw std::runtime_error("Hit array must have 4 columns");

    double *data = static_cast<double *>(hit.data());
    RawHit inputHit = std::make_tuple(data[0], data[1], data[2], data[3]);

    if (!pythonHitMap.count(inputHit))
        throw std::runtime_error("No hit exists with the given position");

    HepEVD::Hit *hepEVDHit = pythonHitMap[inputHit];

    for (auto item : properties) {
        std::string key = nb::cast<std::string>(item.first);
        double value = nb::cast<double>(item.second);

        hepEVDHit->addProperties({{key, value}});
    }
}

NB_MODULE(hep_evd_two, m) {

    m.doc() = "HepEVD - High Energy Physics Event Display";

    m.def("start_server", &HepEVD::startServer, "Starts the HepEVD server", nb::arg("start_state") = -1,
          nb::arg("clear_on_show") = true);
    m.def("is_initialised", &HepEVD::isServerInitialised,
          "Checks if the server is initialised - i.e. does a server exists, with the geometry set?");
    m.def("set_verbose", &HepEVD::setVerboseLogging, "Sets the verbosity of the HepEVD server", nb::arg("verbose"));

    m.def("save_state", &HepEVD::saveState, "Saves the current state", nb::arg("state_name"), nb::arg("min_size") = -1,
          nb::arg("clear_on_show") = true);
    m.def("reset_server", &HepEVD::resetServer, "Resets the server", nb::arg("reset_geo") = false);

    m.def("set_geometry", &set_geometry, "Sets the geometry of the server", nb::arg("geometry"));

    m.def("add_hits", &add_hits<HepEVD::Hit>,
          "Adds hits to the current event state. Hits must be passed an (N, 4) list or array, with the 4 columns being "
          "(x, y, z, energy)",
          nb::arg("hits"), nb::arg("label") = "");
    m.def("add_mc", &add_hits<HepEVD::MCHit>,
          "Adds hits to the current event state. Hits must be passed an (N, 5) list or array, with the 5 columns being "
          "(x, y, z, energy, PDG)",
          nb::arg("mcHits"), nb::arg("label") = "");
    m.def("add_hit_properties", &set_hit_properties, "Add custom properties to a hit, via a string / double dictionary",
          nb::arg("hit"), nb::arg("properties"));

    nb::enum_<HepEVD::HitType>(m, "HitType")
        .value("GENERAL", HepEVD::GENERAL)
        .value("TWO_D_U", HepEVD::TWO_D_U)
        .value("TWO_D_V", HepEVD::TWO_D_V)
        .value("TWO_D_W", HepEVD::TWO_D_W);

    nb::enum_<HepEVD::HitDimension>(m, "HitDimension").value("TWO_D", HepEVD::TWO_D).value("THREE_D", HepEVD::THREE_D);
}