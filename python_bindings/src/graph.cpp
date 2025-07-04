//
// Helper function to visualise graphs in HepEVD.
//

// Standard includes
#include <iostream>
#include <map>
#include <vector>

// Include the HepEVD header files.
#define HEP_EVD_BASE_HELPER 1
#include "hep_evd.h"

// Local Includes
#include "include/array_list_utils.hpp"
#include "include/global.hpp"
#include "include/graph.hpp"

// Include nanobind
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

namespace HepEVD_py {

void add_graph(nb::handle nodes, nb::handle edges, nb::handle nodeColours, nb::handle edgeColours, std::string label) {

    if (!HepEVD::isServerInitialised())
        return;

    if (!isArrayOrList(nodes) || !isArrayOrList(edges))
        throw std::runtime_error("HepEVD: Graph nodes and edges must be arrays or lists");

    BasicSizeInfo nodesSize = getBasicSizeInfo(nodes);
    BasicSizeInfo nodeColoursSize = getBasicSizeInfo(nodeColours);
    bool hasNodeColours = !nodeColours.is_none();

    BasicSizeInfo edgesSize = getBasicSizeInfo(edges);
    BasicSizeInfo edgeColoursSize = getBasicSizeInfo(edgeColours);
    bool hasEdgeColours = !edgeColours.is_none();

    // We expect the following shapes for the input arrays:
    // - A 2D array for nodes: (N, 3) where N is the number of nodes.
    // - A 2D array for edges: (M, 2) where M is the number of edges.
    if (nodesSize.size() != 2 || nodesSize[1] != 3)
        throw std::runtime_error("HepEVD: Graph positions must be a 2D array with shape (N, 3)");

    if (edgesSize.size() != 2 || edgesSize[1] != 2)
        throw std::runtime_error("HepEVD: Graph edges must be a 2D array with shape (M, 2)");

    if (nodeColoursSize.size() > 0 && nodeColoursSize[0] != nodesSize[0])
        throw std::runtime_error("HepEVD: Node colours must match the number of nodes");

    if (edgeColoursSize.size() > 0 && edgeColoursSize[0] != edgesSize[0])
        throw std::runtime_error("HepEVD: Line colours must match the number of edges");

    HepEVD::Markers markers;

    bool isThreeD = (nodesSize.size() == 2 && nodesSize[1] == 3);
    HepEVD::HitDimension hitDim = isThreeD ? HepEVD::HitDimension::THREE_D : HepEVD::HitDimension::TWO_D;
    int hitSize = isThreeD ? 3 : 2;

    int numNodes = nodesSize[0];
    int numEdges = edgesSize[0];

    for (int nodeIdx = 0; nodeIdx < numNodes; nodeIdx++) {

        float x = nb::cast<float>(nodes[nodeIdx][0]);
        float y = nb::cast<float>(nodes[nodeIdx][1]);
        float z = isThreeD ? nb::cast<float>(nodes[nodeIdx][2]) : 0.0f;

        HepEVD::Point point({x, y, z});
        point.setDim(hitDim);

        if (hasNodeColours) {
            auto colour = nb::cast<std::string>(nodeColours[nodeIdx]);
            point.setColour(colour);
        } else {
            point.setColour("blue");
        }

        point.setLabel(label);

        markers.push_back(point);
    }

    for (int edgeIdx = 0; edgeIdx < numEdges; edgeIdx++) {
        auto edge = nb::cast<std::array<int, 2>>(edges[edgeIdx]);

        HepEVD::Point startPoint = std::get<HepEVD::Point>(markers[edge[0]]);
        HepEVD::Point endPoint = std::get<HepEVD::Point>(markers[edge[1]]);

        HepEVD::Line line(startPoint, endPoint);

        if (hasEdgeColours) {
            auto colour = nb::cast<std::string>(edgeColours[edgeIdx]);
            line.setColour(colour);
        } else {
            line.setColour("grey");
        }

        line.setLabel(label);
        markers.push_back(line);
    }

    HepEVD::hepEVDLog("Adding " + std::to_string(markers.size()) + " graph markers to the HepEVD server.");
    HepEVD::getServer()->addMarkers(markers);
}

} // namespace HepEVD_py
