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
struct surface_converter {
    template <typename mask_group_t, typename index_t, typename transform_t>
    inline auto operator()(const mask_group_t &mask_group, const index_t &index, const transform_t &transform) const {

        const auto vertices(mask_group[index].vertices(10u));
        Positions positions;
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            const auto vertex(transform.point_to_global(vertices[i]));
            Position pos({(double)vertex[0], (double)vertex[1], (double)vertex[2]});
            positions.push_back(pos);
        }

        return positions;
    }
};
} // namespace

// Set the HepEVD geometry by pulling the relevant information from the
// detray detector_t object.
// We need this to be templated on the detector_t object, as the detray
// detector_t object can be different for each detector.
template <typename detector_t> static void setHepEVDGeometry(const detector_t &detector) {

    if (isServerInitialised())
        return;

    Volumes volumes;
    std::map<std::string, int> missedSurfacesTypes;

    int volume_id(0);
    for (const auto &vol_desc : detector.volumes()) {
        ++volume_id;
        int surface_id(0);
        const auto volume = detray::detector_volume{detector, vol_desc};
        for (const auto &surf_desc : volume.surfaces()) {
            const auto surface = detray::surface{detector, surf_desc};

            const auto shape_name(surface.shape_name());

            auto gctx(typename detector_t::geometry_context{});
            // const auto centroid(surface.transform(gctx).point_to_global(surface.centroid()));
            const auto centroid(surface.center(gctx));
            const Position position({(double)centroid[0], (double)centroid[1], (double)centroid[2]});

            if (shape_name == "trapezoid2D") {
                const auto vertices(surface.template visit_mask<surface_converter>(surface.transform(gctx)));
                TrapezoidVolume trapezoid(position, vertices);
                volumes.push_back(trapezoid);
            } else if (shape_name == "rectangle2D") {
                const auto vertices(surface.template visit_mask<surface_converter>(surface.transform(gctx)));
                Rectangle2DVolume rect(position, vertices);
                volumes.push_back(rect);
            } else {
                missedSurfacesTypes[shape_name]++;
            }

            surface_id++;
        }
    }

    for (const auto &missedSurface : missedSurfacesTypes) {
        std::cout << "Missed surface type: " << missedSurface.first << " (" << missedSurface.second << " surfaces)"
                  << std::endl;
    }

    hepEVDServer = new HepEVDServer(DetectorGeometry(volumes));
}

// Add traccc::Spacepoints to the HepEVD server.
static void addSpacepoints(const vecmem::data::vector_view<traccc::spacepoint> &spacePoints, std::string label = "") {

    if (!isServerInitialised())
        return;

    Hits hits;

    for (unsigned int i = 0; i < spacePoints.size(); i++) {
        const auto spacePoint = spacePoints.ptr()[i];
        Hit *hit = new Hit({spacePoint.x(), spacePoint.y(), spacePoint.z()}, 0.0);

        if (label != "")
            hit->setLabel(label);

        hit->setDim(THREE_D);
        hits.push_back(hit);
    }

    hepEVDServer->addHits(hits);
}

}; // namespace HepEVD

#endif // HEP_EVD_TRACCC_HELPERS_H
