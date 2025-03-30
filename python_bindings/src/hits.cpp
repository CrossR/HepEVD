//
// Hit-based functions for the HepEVD Python Bindings
//

// Standard includes
#include <map>
#include <vector>

// Include the HepEVD header files.
#define HEP_EVD_BASE_HELPER 1
#include "hep_evd.h"

// Local Includes
#include "include/array_list_utils.hpp"
#include "include/global.hpp"
#include "include/hits.hpp"

// Include nanobind
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

namespace HepEVD_py {

template <typename T> void add_hits(nb::handle hits, std::string label) {

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
            hit = new T(HepEVD::Position({x, y, z}), pdgCode, energy);
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

    if constexpr (std::is_same<T, HepEVD::MCHit>::value) {
        HepEVD::hepEVDLog("Adding " + std::to_string(hepEVDHits.size()) + " MC hits to the HepEVD server.");
        HepEVD::getServer()->addMCHits(hepEVDHits);
    } else {
        HepEVD::hepEVDLog("Adding " + std::to_string(hepEVDHits.size()) + " hits to the HepEVD server.");
        HepEVD::getServer()->addHits(hepEVDHits);
    }
}

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

// Instantiate the templated functions.
template void add_hits<HepEVD::Hit>(nb::handle hits, std::string label);
template void add_hits<HepEVD::MCHit>(nb::handle hits, std::string label);

} // namespace HepEVD_py
