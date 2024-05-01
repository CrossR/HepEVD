//
// Detray Helpers
//
// High-level helpers to view traccc / detray objects in HepEVD.

#ifndef HEP_EVD_TRACCC_HELPERS_H
#define HEP_EVD_TRACCC_HELPERS_H

#include <map>
#include <string>

// Detray Includes
#include "detray/geometry/detector_volume.hpp"
#include "detray/geometry/surface.hpp"

// Local Includes
#include "base_helper.h"
#include "geometry.h"
#include "hits.h"
#include "particle.h"
#include "server.h"
#include "utils.h"

namespace HepEVD {

// First, we need to define a mapping between the various detray objects
// and the HepEVD objects.

namespace {
/// @brief A functor to set the proto surfaces type and bounds to be equivalent
/// to the mask.
struct surface_converter {
    template <typename mask_group_t, typename index_t, typename transform_t>
    inline auto operator()(const mask_group_t& mask_group,
                           const index_t& index,
                           const transform_t& transform) const {

        const auto vertices(mask_group[index].vertices(10u));
        Positions positions;
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            const auto vertex(transform.point_to_global(vertices[i]));
            Position pos({(double) vertex[0], (double) vertex[1], (double) vertex[2]});
            positions.push_back(pos);
        }

        return positions;
    }
};
}  // namespace

// Set the HepEVD geometry by pulling the relevant information from the
// detray detector_t object.
// We need this to be templated on the detector_t object, as the detray
// detector_t object can be different for each detector.
template<typename detector_t>
static void setHepEVDGeometry(const detector_t& detector) {

    if (isServerInitialised())
        return;

    Volumes volumes;

    int volume_id(0);
    for (const auto &vol_desc : detector.volumes()) {
        std::cout << "Volume ID: " << volume_id << std::endl;
        ++volume_id;
        int surface_id(0);
        const auto volume = detray::detector_volume{detector, vol_desc};
        for (const auto &surf_desc : volume.surfaces()) {
            const auto surface = detray::surface{detector, surf_desc};

            // TODO: Deal with the relevant shapes...o
            // : trapezoid2D, cylinder2D, ring2D
            std::cout << surface.shape_name();

            if (surface.is_portal()) {
                std::cout << " : Portal" << std::endl;
            } else {
                std::cout << " : Not a portal" << std::endl;
            }

            auto gctx(typename detector_t::geometry_context{});
            const auto centroid(surface.transform(gctx).point_to_global(surface.centroid()));
            const Position position({(double) centroid[0], (double) centroid[1], (double) centroid[2]});

            if (surface.shape_name() == "trapezoid2D") {
                const auto vertices(surface.template visit_mask<surface_converter>(surface.transform(gctx)));
                TrapezoidVolume trapezoid(position, vertices);
                volumes.push_back(trapezoid);
            }

            surface_id++;
        }
    }

    hepEVDServer = new HepEVDServer(DetectorGeometry(volumes));
    hepEVDServer->startServer();
}

}; // namespace HepEVD

#endif // HEP_EVD_TRACCC_HELPERS_H
