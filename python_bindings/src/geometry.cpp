//
// Geometry functions for the HepEVD Python Bindings
//

// Standard includes
#include <vector>

// Include the HepEVD header files.
#define HEP_EVD_BASE_HELPER 1
#include "hep_evd.h"

// Local Includes
#include "include/array_list_utils.hpp"
#include "include/detectors.hpp"
#include "include/geometry.hpp"
#include "include/global.hpp"

// Include nanobind
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

namespace HepEVD_py {

// Set the current HepEVD geometry.
// Input will either be a string or a list/array of numbers.
void set_geometry(nb::object geometry) {

    if (HepEVD::isServerInitialised(true))
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

    HepEVD::hepEVDLog("Setting HepEVD geometry with " + std::to_string(volumes.size()) + " volumes.");
    hepEVDServer = new HepEVDServer(DetectorGeometry(volumes));

    // Register the clear function for the hit map,
    // so we can clear it when we need to.
    HepEVD::registerClearFunction([&]() { pythonHitMap.clear(); });
}

} // namespace HepEVD_py
