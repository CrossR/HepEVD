//
// Marker functions for the HepEVD Python Bindings
//

// Standard includes
#include <map>
#include <vector>

// Include the HepEVD header files.
#define HEP_EVD_BASE_HELPER 1
#include "hep_evd.h"

// Local Includes
#include "include/array_list_utils.hpp"
#include "include/markers.hpp"

// Include nanobind
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

namespace HepEVD_py {

void init_marker_classes(nb::module_ &m) {

    nb::class_<HepEVD::Position>(m, "Position")
        .def(nb::init<>())
        .def(nb::init<std::array<double, 3>>())
        .def_rw("x", &HepEVD::Position::x)
        .def_rw("y", &HepEVD::Position::y)
        .def_rw("z", &HepEVD::Position::z)
        .def_rw("dim", &HepEVD::Position::dim)
        .def_rw("hitType", &HepEVD::Position::hitType);

    // Marker
    nb::class_<HepEVD::Marker>(m, "Marker")
        .def(nb::init<>())
        .def(nb::init<std::array<double, 3>>())
        .def("set_colour", &HepEVD::Marker::setColour)
        .def("set_label", &HepEVD::Marker::setLabel)
        .def("set_dim", &HepEVD::Marker::setDim)
        .def("set_hit_type", &HepEVD::Marker::setHitType);

    // Point
    nb::class_<HepEVD::Point, HepEVD::Marker>(m, "Point")
        .def(nb::init<>())
        .def(nb::init<std::array<double, 3>>())
        .def(nb::init<std::array<double, 3>, HepEVD::HitDimension, HepEVD::HitType>());

    // Line
    nb::class_<HepEVD::Line, HepEVD::Marker>(m, "Line").def(nb::init<>()).def(nb::init<HepEVD::Point, HepEVD::Point>());

    // Ring
    nb::class_<HepEVD::Ring, HepEVD::Marker>(m, "Ring")
        .def(nb::init<>())
        .def(nb::init<std::array<double, 3>, double, double>());
}

void add_markers(nb::handle markers) {

    if (!HepEVD::isServerInitialised())
        return;

    if (!isArrayOrList(markers))
        throw std::runtime_error("HepEVD: Markers must be an array or list");

    BasicSizeInfo markersSize = getBasicSizeInfo(markers);

    if (markersSize.size() != 1)
        throw std::runtime_error("HepEVD: Markers array must be 1D");

    HepEVD::Markers hepEVDMarkers;

    // For every marker, try casting it to the correct type
    for (int i = 0; i < markersSize[0]; i++) {
        const nb::handle marker = markers[i];

        if (nb::isinstance<HepEVD::Point>(marker))
            hepEVDMarkers.push_back(nb::cast<HepEVD::Point>(marker));
        else if (nb::isinstance<HepEVD::Line>(marker))
            hepEVDMarkers.push_back(nb::cast<HepEVD::Line>(marker));
        else if (nb::isinstance<HepEVD::Ring>(marker))
            hepEVDMarkers.push_back(nb::cast<HepEVD::Ring>(marker));
        else
            throw std::runtime_error("HepEVD: Unknown marker type");
    }

    // Finally, add the markers
    HepEVD::addMarkers(hepEVDMarkers);
}

} // namespace HepEVD_py