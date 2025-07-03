//
// Particle-based functions for the HepEVD Python Bindings
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
#include "include/hits.hpp"

// Include nanobind
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

namespace HepEVD_py {

void add_particles(nb::handle particles, std::string label) {

    if (!HepEVD::isServerInitialised())
        return;

    if (!isArrayOrList(particles))
        throw std::runtime_error("HepEVD: Particles must be an array or list");

    BasicSizeInfo arraySize = getBasicSizeInfo(particles);

    // For now, we only a flat list of particles.
    // That is:
    // - A 3D array: (P, H, 4) where P is the number of particles and H is the number of hits per particle.
    // TODO: Ideally, we want to support any number of levels of nesting, such
    // that we can have complex parent-child relationships.
    // For now, we will just assume a flat list of particles.
    if (arraySize.size() != 3)
        throw std::runtime_error(
            "HepEVD: Particles array must be 3D!\n "
            "Expected shape (P, H, 4) where P is the number of particles and H is the number of hits per particle.\n");

    HepEVD::Particles hepEVDParticles;

    int numParticles = arraySize[0];

    for (int particleIdx = 0; particleIdx < numParticles; particleIdx++) {

        // Get the hits for this particle.
        BasicSizeInfo particleSize = getBasicSizeInfo(particles[particleIdx]);
        int numHits = particleSize[0];

        HepEVD::Hits particleHits;

        for (int hitIdx = 0; hitIdx < numHits; hitIdx++) {

            // Get the hit for this particle.
            nb::handle hitHandle = particles[particleIdx][hitIdx];

            // Check that the hit is a list or array.
            if (!isArrayOrList(hitHandle))
                throw std::runtime_error("HepEVD: Hit must be an array or list");

            BasicSizeInfo hitSize = getBasicSizeInfo(hitHandle);

            // Each hit should be a 1D array with 4+ elements...
            if (hitSize.size() != 1 || hitSize[0] < 4)
                throw std::runtime_error("HepEVD: Hit must be a 1D array, with at least 4 elements (x, y, z, energy)");

            // Get hit data
            auto hitData = getItems(hitHandle, 0, hitSize[0]);
            int dataSize = hitSize[0];

            auto idx(0);
            double x = hitData[idx++];
            double y = hitData[idx++];
            double z = hitData[idx++];
            double energy = hitData[idx++];

            // Optional view and dimension.
            bool includesView = dataSize >= 6;
            double dimension = includesView ? hitData[idx++] : -1.0;
            double view = includesView ? hitData[idx++] : -1.0;

            HepEVD::Hit *hepHit = new HepEVD::Hit({x, y, z}, energy);

            if (includesView) {
                hepHit->setHitType(static_cast<HepEVD::HitType>(view));
                hepHit->setDim(static_cast<HepEVD::HitDimension>(dimension));
            }

            if (!label.empty())
                hepHit->setLabel(label);

            particleHits.push_back(hepHit);
        }

        // Now, we can create a HepEVD particle object.
        std::string id(HepEVD::getUUID());
        HepEVD::Particle *hepParticle = new HepEVD::Particle(particleHits, id, label);
        hepEVDParticles.push_back(hepParticle);
    }

    // Finally, we can add the particles to the HepEVD server.
    HepEVD::hepEVDLog("Adding " + std::to_string(hepEVDParticles.size()) + " particles to the HepEVD server.");
    HepEVD::getServer()->addParticles(hepEVDParticles);
}

} // namespace HepEVD_py
