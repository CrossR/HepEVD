//
// Detray Helpers
//
// High-level helpers to view traccc / detray objects in HepEVD.

#ifndef HEP_EVD_TRACCC_HELPERS_H
#define HEP_EVD_TRACCC_HELPERS_H

#include <map>
#include <string>

// Detray Includes
#include "detray/core/detector.hpp"
#include "detray/geometry/tracking_surface.hpp"
#include "detray/geometry/tracking_volume.hpp"

// traccc Includes
#include "traccc/edm/seed_collection.hpp"
#include "traccc/edm/spacepoint_collection.hpp"
#include "traccc/edm/track_candidate.hpp"
#include "traccc/edm/track_state.hpp"

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

    if (isServerInitialised(true))
        return;

    Volumes volumes;
    std::map<std::string, int> missedSurfacesTypes;

    int volume_id(0);
    for (const auto &vol_desc : detector.volumes()) {
        ++volume_id;
        int surface_id(0);
        const auto volume = detray::tracking_volume{detector, vol_desc};
        for (const auto &surf_desc : volume.surfaces()) {

            const auto surface = detray::tracking_surface{detector, surf_desc};
            const auto shape_name(surface.shape_name());
            auto gctx(typename detector_t::geometry_context{});

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

    hepEVDLog("Adding " + std::to_string(volumes.size()) + " volumes to the HepEVD server.");
    hepEVDServer = new HepEVDServer(DetectorGeometry(volumes));

    // Since this function needs to be called for HepEVD to work with traccc, lets also do some quick setup.
    // Lets set a few GUI options, just to make our events look a bit nicer, as compared to the more LArTPC
    // focused defaults.
    hepEVDServer->getConfig()->hits.size = 10.0;
    hepEVDServer->getConfig()->hits.colour = "red";
    // There is no need to show the 2D view, as we are not using it.
    // Could later be updated to have some 2D projections?
    hepEVDServer->getConfig()->show2D = false;
    // No need for mouse over interactions, as they are slow in the very high pileup events, and not
    // very useful.
    hepEVDServer->getConfig()->disableMouseOver = true;
    // Similarly, the particle menu can have so many entries in the high pileup events, that it
    // is not very useful. As it is not a virtualised list, it is also very slow.
    hepEVDServer->getConfig()->showParticleMenu = false;
}

// Add traccc::Spacepoints to the HepEVD server.
static void addSpacepoints(const traccc::edm::spacepoint_collection::const_view &spacePoints, std::string label = "") {

    if (!isServerInitialised())
        return;

    Hits hits;

    const traccc::edm::spacepoint_collection::const_device spacePointsView{spacePoints};

    for (unsigned int i = 0; i < spacePointsView.size(); i++) {
        const auto spacePoint = spacePointsView.at(i);
        Hit *hit = new Hit({spacePoint.x(), spacePoint.y(), spacePoint.z()}, 0.0);

        if (label != "")
            hit->setLabel(label);

        hit->setDim(THREE_D);
        hits.push_back(hit);
    }

    hepEVDLog("Adding " + std::to_string(hits.size()) + " spacepoints to the HepEVD server.");
    hepEVDServer->addHits(hits);
}

// Add traccc::seeds to the HepEVD server.
static void addSeeds(const traccc::edm::seed_collection::const_view &seeds,
                     const traccc::edm::spacepoint_collection::const_view &spacePoints, std::string label = "") {

    if (!isServerInitialised())
        return;

    Particles hepSeeds;

    // Create a device collection around the seed container view.
    const traccc::edm::seed_collection::const_device seedsView{seeds};
    const traccc::edm::spacepoint_collection::const_device spacePointsView{spacePoints};

    for (unsigned int i = 0; i < seedsView.size(); i++) {
        const auto seed = seedsView.at(i);
        Hits hits;

        for (const auto &index : {seed.bottom_index(), seed.middle_index(), seed.top_index()}) {
            const auto spacePoint = spacePointsView.at(index);
            Hit *hit = new Hit({spacePoint.x(), spacePoint.y(), spacePoint.z()}, 0.0);
            hits.push_back(hit);
        }

        std::string id = getUUID();
        Particle *particle = new Particle(hits, id, label);

        hepSeeds.push_back(particle);
    }

    hepEVDLog("Adding " + std::to_string(hepSeeds.size()) + " seeds to the HepEVD server.");
    hepEVDServer->addParticles(hepSeeds);
}

// Add track candidates to the HepEVD server
template <typename detector_t>
static void addTrackCandidates(const traccc::track_candidate_container_types::const_view &tracks_view,
                               const detector_t &detector, std::string label = "") {

    if (!isServerInitialised())
        return;

    Particles hepTracks;

    // Create a device collection around the track container view.
    const traccc::track_candidate_container_types::const_device tracks{tracks_view};

    for (unsigned int i = 0; i < tracks.size(); i++) {

        // The track candidate in question.
        const traccc::track_candidate_container_types::const_device::const_element_view track = tracks.at(i);

        // Start to build the hits for this track candidate.
        Hits hits;

        // Loop over the measurements in the track candidate.
        for (const traccc::measurement &m : track.items) {

            // Check the measurement isn't empty
            if (m.local[0] == 0 && m.local[1] == 0 && m.local[2] == 0) {
                continue;
            }

            // Find the detector surface that this measurement sits on.
            const detray::tracking_surface<detector_t> surface{detector, m.surface_link};

            // Calculate a position for this measurement in global 3D space.
            const auto global = surface.bound_to_global({}, m.local, {});

            // Create a hit object for this measurement.
            Hit *hit = new Hit({global[0], global[1], global[2]}, 0.0);
            hits.push_back(hit);
        }

        // Create a particle object for this track candidate.
        std::string id = getUUID();
        Particle *particle = new Particle(hits, id, label);
        particle->setRenderType(RenderType::TRACK);

        // Add the particle to the list of particles to be added to the server.
        hepTracks.push_back(particle);
    }

    // Add the particles to the server.
    hepEVDLog("Adding " + std::to_string(hepTracks.size()) + " track candidates to the HepEVD server.");
    hepEVDServer->addParticles(hepTracks);
}

template <typename detector_t>
static void addTracks(const traccc::track_state_container_types::const_view &tracks_view, const detector_t &detector,
                      std::string label = "") {

    if (!isServerInitialised())
        return;

    Particles hepTracks;

    // Create a device collection around the track container view.
    const traccc::track_state_container_types::const_device tracks{tracks_view};

    for (unsigned int i = 0; i < tracks.size(); i++) {

        // The track state in question.
        const traccc::track_state_container_types::const_device::const_element_view track = tracks.at(i);

        // Start to build the hits for this track candidate.
        Hits hits;

        // Loop over the measurements in the track candidate.
        for (const traccc::track_state ts : track.items) {

            // Find the detector surface that this measurement sits on.
            const auto m = ts.get_measurement();
            const detray::tracking_surface<detector_t> surface{detector, m.surface_link};

            // Calculate a position for this measurement in global 3D space.
            const auto global = surface.bound_to_global({}, m.local, {});

            // Create a hit object for this measurement.
            Hit *hit = new Hit({global[0], global[1], global[2]}, 0.0);
            hits.push_back(hit);
        }

        // Create a particle object for this track candidate.
        std::string id = getUUID();
        Particle *particle = new Particle(hits, id, label);
        particle->setRenderType(RenderType::TRACK);

        // Add the particle to the list of particles to be added to the server.
        hepTracks.push_back(particle);
    }

    // Add the particles to the server.
    hepEVDLog("Adding " + std::to_string(hepTracks.size()) + " tracks to the HepEVD server.");
    hepEVDServer->addParticles(hepTracks);
}

}; // namespace HepEVD

#endif // HEP_EVD_TRACCC_HELPERS_H
