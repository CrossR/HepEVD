//
// Detector Geometry Definitions
//
// All of these are based on the Pandora Geometry files from LArReco.

#ifndef DETECTORS_HPP
#define DETECTORS_HPP

#include <map>
#include <string>

#include "hep_evd.h"

using namespace HepEVD;
using GeoNamePair = std::pair<std::string, Volumes>;

// DUNE FD 1x2x6
GeoNamePair dune_fd_1x2x6 = {
    "dunefd_1x2x6",
    {BoxVolume(Position({-182.954544067, 0.0, 696.293762207}), 359.415008545, 1207.84753418, 1394.33996582),
     BoxVolume(Position({182.954544067, 0.0, 696.293762207}), 359.415008545, 1207.84753418, 1394.33996582)}};

// DUNE FD 1x8x6 VD
GeoNamePair dune_fd_1x8x6_vd = {
    "dunefd_1x8x6_vd", {BoxVolume(Position({0.0400085449219, 0.0, 448.260009766}), 650, 1344.6048584, 895.520019531)}};

// ProtoDUNE HD
GeoNamePair protodune = {
    "protodune",
    {
        BoxVolume(Position({-371.867614746, 303.749389648, 347.396240234}), 8.49124145508, 600.223754883,
                  695.780029297),
        BoxVolume(Position({-179.715103149, 303.749389648, 347.396240234}), 357.626251221, 600.223754883,
                  695.780029297),
        BoxVolume(Position({179.715103149, 303.749389648, 347.396240234}), 357.626251221, 600.223754883, 695.780029297),
        BoxVolume(Position({371.867614746, 303.749389648, 347.396240234}), 8.49124145508, 600.223754883, 695.780029297),
    }};

// ProtoDUNE VD
GeoNamePair protodune_vd = {"protodune_vd",
                            {
                                BoxVolume(Position({0.214996337891, 0.0, 300.5}), 597, 601, 601),
                            }};

// SBND
GeoNamePair sbnd = {
    "sbnd",
    {
        BoxVolume(Position({-101.099998474, 0.0, 254.699996948}), 201.300003052, 407.464508057, 509.399993896),
        BoxVolume(Position({101.099998474, 0.0, 254.699996948}), 201.300003052, 407.464508057, 509.399993896),
    }};

// MicroBooNE
GeoNamePair microboone = {
    "microboone",
    {
        BoxVolume(Position({128.175003052, 0.970001220703, 518.5}), 256.350006104, 233, 1036.80004883),
    }};

// Top-level map
std::map<std::string, Volumes> detectors = {dune_fd_1x2x6, dune_fd_1x8x6_vd, protodune, protodune_vd,
                                            sbnd,          microboone

};

#endif