#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>

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

// We want to catch SIGINT, SIGTERM and SIGKILL and shut down the server
// when that happens.
//
// But we also don't want to interfere with other signals, when the server
// is not running.
//
// So setup and teardown the signals here, around the server starting and finishing.
typedef void (*sighandler_t)(int);
std::vector<sighandler_t> catch_signals() {
    auto handler = [](int code) {
        if (hepEVDServer != nullptr) {
            std::cout << "HepEVD: Caught signal " << code << ", shutting down." << std::endl;
            hepEVDServer->stopServer();
        }
        exit(0);
    };

    std::vector<sighandler_t> oldHandlers;

    oldHandlers.push_back(signal(SIGINT, handler));
    oldHandlers.push_back(signal(SIGTERM, handler));
    oldHandlers.push_back(signal(SIGKILL, handler));

    return oldHandlers;
}

void revert_signals(std::vector<sighandler_t> oldHandlers) {
    signal(SIGINT, oldHandlers[0]);
    signal(SIGINT, oldHandlers[1]);
    signal(SIGINT, oldHandlers[2]);
}

void start_server_with_signal_handling(const int startState = -1, const bool clearOnShow = true) {
    const auto oldHandlers = catch_signals();
    HepEVD::startServer(startState, clearOnShow);
    revert_signals(oldHandlers);
}

// Map from Python types to HepEVD types.
using RawHit = std::tuple<double, double, double, double>;
using PythonHitMap = std::map<RawHit, HepEVD::Hit *>;
inline PythonHitMap pythonHitMap;

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
            auto data = getItems(geometry, i, 6);
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
    // We are expecting the shape to be (N, 4 or 5) where N is the number of hits.
    //
    // Optionally, there can also be 2 extra columns, one for the dimension and one for the view.
    int expectedSize = std::is_same<T, HepEVD::Hit>::value ? 4 : 5;
    int actualSize = arraySize[1];

    bool includesDimension = false;
    bool includesView = false;

    // Check that the number of columns is correct.  That is, either 4, 5, 6 or
    // 7, corresponding to a regular hit or MC hit, and then 2 additional extra
    // fields for the dimension and view.
    switch (actualSize) {
    case 4:
    case 5:
        break;
    case 6:
    case 7:
        includesDimension = true;
        includesView = true;
        break;
    default:
        throw std::runtime_error("HepEVD: Hits array must have " + std::to_string(expectedSize) + " or " +
                                 std::to_string(expectedSize + 2) + " columns, not " + std::to_string(actualSize));
    }

    int rows = arraySize[0];
    int cols = arraySize[1];

    // Process all the hits in the array.
    std::vector<T *> hepEVDHits;

    for (int i = 0; i < rows; i++) {

        auto data = getItems(hits, i, cols);

        auto idx(0);
        double x = data[idx++];
        double y = data[idx++];
        double z = data[idx++];
        double energy = data[idx++];

        // Optional features
        double pdgCode = std::is_same<T, HepEVD::MCHit>::value ? data[idx++] : -1.0;
        double dimension = includesDimension ? data[idx++] : -1.0;
        double view = includesView ? data[idx++] : -1.0;

        T *hit(nullptr);

        if constexpr (std::is_same<T, HepEVD::MCHit>::value) {
            hit = new T(HepEVD::Position({x, y, z}), energy, pdgCode);
        } else {
            hit = new T(HepEVD::Position({x, y, z}), energy);
            pythonHitMap[std::make_tuple(x, y, z, energy)] = hit;
        }

        // If we have either a dimension or view, add them to the hit.
        if (includesDimension)
            hit->setDim(static_cast<HepEVD::HitDimension>(dimension));

        if (includesView)
            hit->setHitType(static_cast<HepEVD::HitType>(view));

        // Finally, apply the label if one was provided.
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

    auto data = getItems(hit, 0, 4);
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
    m.def("start_server", &start_server_with_signal_handling, "Starts the HepEVD server", nb::arg("start_state") = -1,
          nb::arg("clear_on_show") = true);
    m.def("set_verbose", &HepEVD::setVerboseLogging, "Sets the verbosity of the HepEVD server", nb::arg("verbose"));

    m.def("save_state", &HepEVD::saveState, "Saves the current state", nb::arg("state_name"), nb::arg("min_size") = -1,
          nb::arg("clear_on_show") = true);
    m.def("reset_server", &HepEVD::resetServer, "Resets the server", nb::arg("reset_geo") = false);

    // Set the current HepEVD geometry.
    // Input will either be a string or a list/array of numbers.
    m.def("set_geometry", &set_geometry, "Sets the geometry of the server", nb::arg("geometry"),
          nb::sig("def set_geometry(geometry: typing.Union[str, "
                  "collections.abc.Collection[collections.abc.Collection[float | int]]]) -> None"));

    m.def("add_hits", &add_hits<HepEVD::Hit>,
          "Adds hits to the current event state.\n"
          "Hits must be passed as an (NHits, Y) list or array, with the columns being "
          "(x, y, z, energy) and two optional columns (view, dimension) for the hit type and dimension.\n"
          "The view and dimension values must be from the HepEVD.HitType and HepEVD.HitDimension enums respectively.",
          nb::arg("hits"), nb::arg("label") = "",
          nb::sig("def add_hits(hits: collections.abc.Collection[collections.abc.Collection[float | int | HitType | "
                  "HitDimension]], "
                  "label: str = '') -> None"));
    m.def("add_mc", &add_hits<HepEVD::MCHit>,
          "Adds MC hits to the current event state.\n"
          "Hits must be passed as an (NHits, Y) list or array, with the columns being "
          "(x, y, z, energy, PDG) and two optional columns (view, dimension) for the hit type and dimension.\n"
          "The view and dimension values must be from the HepEVD.HitType and HepEVD.HitDimension enums respectively.",
          nb::arg("mcHits"), nb::arg("label") = "",
          nb::sig("def add_mc(hits: collections.abc.Collection[collections.abc.Collection[float | int | HitType | "
                  "HitDimension]], "
                  "label: str = '') -> None"));
    m.def("add_hit_properties", &set_hit_properties,
          "Add custom properties to a hit, via a string / double dictionary.\n"
          "The hit must be passed as a (x, y, z, energy) list or array.",
          nb::arg("hit"), nb::arg("properties"),
          nb::sig("def add_hit_properties(hit: collections.abc.Collection[float | int], properties: "
                  "typing.Dict[float | int]) "
                  "-> None"));

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
}