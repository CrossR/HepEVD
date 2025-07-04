// graph.hpp

#ifndef HEP_EVD_PY_GRAPH_HPP
#define HEP_EVD_PY_GRAPH_HPP

// Standard includes
#include <string>

// Include nanobind headers
#include <nanobind/nanobind.h>
namespace nb = nanobind;

namespace HepEVD_py {

/**
 * Add the given list/array of graph to the server.
 *
 * @param nodes The handle to the list/array of graph positions.
 * @param edges The handle to the list/array of graph edges.
 * @param nodeColours An optional list/array of colours for the nodes (default: "grey").
 *                   If not provided, all nodes will be blue.
 * @param edgeColours An optional list/array of colours for the edges (default: "blue").
 *                   If not provided, all edges will be grey.
 * @param label The label for the graph (default is empty).
 */
void add_graph(nb::handle nodes, nb::handle edges,
               nb::handle nodeColours = nb::none(),
               nb::handle edgeColours = nb::none(),
               std::string label = "");

} // namespace HepEVD_py

#endif // HEP_EVD_PY_GRAPH_HPP
