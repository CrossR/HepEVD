#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/ndarray.h>

#include <cstdlib>
#include <map>
#include <signal.h>

// Include the HepEVD header files.
#define HEP_EVD_BASE_HELPER 1
#include "hep_evd.h"

// And any local includes...
#include "../include/detectors.hpp"
#include "array_list_utils.cpp"

namespace nb = nanobind;

// Define a signal handler to catch SIGINT, SIGTERM, SIGKILL.
// This is so we can gracefully shutdown the server.
void catch_signals() {
    auto handler = [](int code) {
        if (hepEVDServer != nullptr) {
            std::cout << "HepEVD: Caught signal " << code << ", shutting down." << std::endl;
            hepEVDServer->stopServer();
        }
        exit(0);
    };

    signal(SIGINT, handler);
    signal(SIGTERM, handler);
    signal(SIGKILL, handler);
}


// Map from Python types to HepEVD types.
using RawHit = std::tuple<double, double, double, double>;
using PythonHitMap = std::map<RawHit, HepEVD::Hit *>;
inline PythonHitMap pythonHitMap;

// Check if the server is currently running.
bool is_server_running() { return hepEVDServer->isRunning(); }
void stop_server() { hepEVDServer->stopServer(); }

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
            throw std::runtime_error("HepEVD: Unknown detector: " + nb::cast<std::string>(geometry));

        volumes = detectors[nb::cast<std::string>(geometry)];
    } else if (isArrayOrList(geometry)) {

        BasicSizeInfo arraySize = getBasicSizeInfo(geometry);

        // TODO: Extend this to other geometry types.
        if (arraySize[1] != 6)
            throw std::runtime_error("HepEVD: Geometry array must have 6 columns, not " + std::to_string(arraySize[1]));

        for (int i = 0; i < arraySize[0]; i++) {
            auto data = getItems<double>(geometry, i, 6);
            volumes.push_back(BoxVolume(Position({data[0], data[1], data[2]}), data[3], data[4], data[5]));
        }
    } else {
        throw std::runtime_error("HepEVD: Unknown geometry type, must be string or array");
    }

    hepEVDServer = new HepEVDServer(DetectorGeometry(volumes));

    // Register the clear function for the hit map,
    // so we can clear it when we need to.
    HepEVD::registerClearFunction([&]() { pythonHitMap.clear(); });
}

// Add the given list/array of hits to the server.
// Works for either HepEVD::Hit or HepEVD::MCHit.
template <typename T> void add_hits(nb::handle hits, std::string label = "") {

    if (!HepEVD::isServerInitialised())
        return;

    if (!isArrayOrList(hits))
        throw std::runtime_error("HepEVD: Hit must be an array or list");

    BasicSizeInfo arraySize = getBasicSizeInfo(hits);

    if (arraySize.size() != 2)
        throw std::runtime_error("Hits array must be 2D");

    // Check that the shape of the array is correct.
    // We are expecting the shape to be (N, 4) where N is the number of hits.
    int expectedSize = std::is_same<T, HepEVD::Hit>::value ? 4 : 5;
    if (arraySize[1] != expectedSize)
        throw std::runtime_error("HepEVD: Hits array must have " + std::to_string(expectedSize) + " columns, not " +
                                 std::to_string(arraySize[1]));

    int rows = arraySize[0];
    int cols = arraySize[1];

    // Process all the hits in the array.
    std::vector<T *> hepEVDHits;

    for (int i = 0; i < rows; i++) {

        auto data = getItems<double>(hits, i, cols);
        double x = data[0];
        double y = data[1];
        double z = data[2];
        double energy = data[3];

        T *hit = new T(HepEVD::Position({x, y, z}), energy);

        if constexpr (std::is_same<T, HepEVD::MCHit>::value) {
            hit->setPDG(data[4]);
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
void set_hit_properties(nb::handle hit, nb::dict properties) {

    if (!HepEVD::isServerInitialised())
        return;

    if (!isArrayOrList(hit))
        throw std::runtime_error("HepEVD: Hit must be an array or list");

    BasicSizeInfo hitSize = getBasicSizeInfo(hit);

    if (hitSize.size() != 1)
        throw std::runtime_error("HepEVD: Hit array must be 1D");
    else if (hitSize[0] != 4)
        throw std::runtime_error("HepEVD: Hit array must have 4 columns, not " + std::to_string(hitSize[0]));

    auto data = getItems<double>(hit, 0, 4);
    RawHit inputHit = std::make_tuple(data[0], data[1], data[2], data[3]);

    if (!pythonHitMap.count(inputHit))
        throw std::runtime_error("HepEVD: No hit exists with the given position");

    HepEVD::Hit *hepEVDHit = pythonHitMap[inputHit];

    for (auto item : properties) {
        std::string key = nb::cast<std::string>(item.first);
        double value = nb::cast<double>(item.second);

        hepEVDHit->addProperties({{key, value}});
    }
}

NB_MODULE(_hepevd_impl, m) {

    m.doc() = "HepEVD - High Energy Physics Event Display";

    m.def("is_initialised", &HepEVD::isServerInitialised,
          "Checks if the server is initialised - i.e. does a server exists, with the geometry set?");
    m.def("is_running", &is_server_running, "Checks if the server is running");
    m.def("start_server", &HepEVD::startServer, "Starts the HepEVD server", nb::arg("start_state") = -1,
          nb::arg("clear_on_show") = true);
    m.def("stop_server", &stop_server, "Stops the HepEVD server");
    m.def("set_verbose", &HepEVD::setVerboseLogging, "Sets the verbosity of the HepEVD server", nb::arg("verbose"));

    m.def("save_state", &HepEVD::saveState, "Saves the current state", nb::arg("state_name"), nb::arg("min_size") = -1,
          nb::arg("clear_on_show") = true);
    m.def("reset_server", &HepEVD::resetServer, "Resets the server", nb::arg("reset_geo") = false);

    // Set the current HepEVD geometry.
    // Input will either be a string or a list/array of numbers.
    m.def("set_geometry", &set_geometry, "Sets the geometry of the server", nb::arg("geometry"),
          nb::sig("def set_geometry(Union[str, List[float]]) -> None"));

    m.def("add_hits", &add_hits<HepEVD::Hit>,
          "Adds hits to the current event state. Hits must be passed an (N, 4) list or array, with the 4 columns being "
          "(x, y, z, energy)",
          nb::arg("hits"), nb::arg("label") = "",
          nb::sig("def add_hits(Union[List[List[float]], Array[Array[float]], Optional[str]]) -> None"));
    m.def("add_mc", &add_hits<HepEVD::MCHit>,
          "Adds hits to the current event state. Hits must be passed an (N, 5) list or array, with the 5 columns being "
          "(x, y, z, energy, PDG)",
          nb::arg("mcHits"), nb::arg("label") = "",
          nb::sig("def add_mc(Union[List[List[float]], Array[Array[float]], Optional[str]]) -> None"));
    m.def("add_hit_properties", &set_hit_properties, "Add custom properties to a hit, via a string / double dictionary",
          nb::arg("hit"), nb::arg("properties"),
          nb::sig("def add_hit_properties(Union[List[float], Array[float]], Dict[str, float]) -> None"));

    nb::enum_<HepEVD::HitType>(m, "HitType")
        .value("GENERAL", HepEVD::GENERAL)
        .value("TWO_D_U", HepEVD::TWO_D_U)
        .value("TWO_D_V", HepEVD::TWO_D_V)
        .value("TWO_D_W", HepEVD::TWO_D_W);

    nb::enum_<HepEVD::HitDimension>(m, "HitDimension").value("TWO_D", HepEVD::TWO_D).value("THREE_D", HepEVD::THREE_D);

    // Add a submodule to store internal functions.
    nb::module_ m2 = m.def_submodule("_internal", "Internal functions");
    m2.def("catch_signals", &catch_signals, "Catches signals");
}