//
// LArSoft Helpers
//
// High-level helpers, optionally included with LArSoft.
// Given a LArSoft module handle, these helpers can be used to
// easily add data to the HepEVD server.

#ifndef HEP_EVD_LARSOFT_HELPERS_H
#define HEP_EVD_LARSOFT_HELPERS_H

#include <map>
#include <string>

// Core Includes
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "canvas/Persistency/Common/FindOneP.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// Various LArSoft Objects...
// Detector..
#include "larcore/Geometry/Geometry.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "lardataobj/AnalysisBase/BackTrackerMatchingData.h"
// Reco...
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/SpacePoint.h"
#include "lardataobj/RecoBase/Vertex.h"
// MC...
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"

// Local Includes
#include "base_helper.h"
#include "geometry.h"
#include "hits.h"
#include "particle.h"
#include "server.h"
#include "utils.h"

namespace HepEVD {

// LArSoft requires a lot more use of various objects for
// converting things, so it is sometimes easier to keep
// a pointer to them, to ease passing around functions.
inline geo::Geometry const *hepEvdLArSoftGeo = nullptr;
inline detinfo::DetectorPropertiesData const *hepEVDDetProps = nullptr;

using RecoHitMap = std::map<const art::Ptr<recob::Hit>, Hit *>;
inline RecoHitMap recoHitToEvdHit;

using SpacePointHitMap = std::map<const art::Ptr<recob::SpacePoint>, Hit *>;
inline SpacePointHitMap spacePointToEvdHit;

// Get the current hit maps, such that properties and more can be added
// to the HepEVD hits.
static RecoHitMap *getHitMap() { return &recoHitToEvdHit; }
static SpacePointHitMap *getSpacePointMap() { return &spacePointToEvdHit; }

// Set the HepEVD geometry by pulling the relevant information from the
// Pandora GeometryManager.
// Also register the map to the manager, so we don't leak memory.
static void setHepEVDGeometry() {

    if (isServerInitialised())
        return;

    // Store for later use.
    // For example, for most recob::Hit use cases, we need to convert
    // the wire position to a real position, using the geometry.
    art::ServiceHandle<geo::Geometry const> geometry;
    hepEvdLArSoftGeo = &*geometry;

    Volumes volumes;

    for (const auto &cryostat : geometry->Iterate<geo::CryostatGeo>()) {
        for (const auto &tpc1 : geometry->Iterate<geo::TPCGeo>(cryostat.ID())) {

            const auto tpc1Bounds = tpc1.ActiveBoundingBox();
            float driftMinX(tpc1Bounds.MinX()), driftMaxX(tpc1Bounds.MaxX());
            float driftMinY(tpc1Bounds.MinY()), driftMaxY(tpc1Bounds.MaxY());
            float driftMinZ(tpc1Bounds.MinZ()), driftMaxZ(tpc1Bounds.MaxZ());

            const double min1((0.5 * (driftMinX + driftMaxX)) - 0.25 * std::fabs(driftMaxX - driftMinX));
            const double max1((0.5 * (driftMinX + driftMaxX)) + 0.25 * std::fabs(driftMaxX - driftMinX));

            for (const auto &tpc2 : geometry->Iterate<geo::TPCGeo>(cryostat.ID())) {

                if (tpc1.DriftDirection() != tpc2.DriftDirection())
                    continue;

                const auto tpc2Bounds = tpc2.ActiveBoundingBox();
                const float driftMinX2(tpc2Bounds.MinX()), driftMaxX2(tpc2Bounds.MaxX());
                const float driftMinY2(tpc2Bounds.MinY()), driftMaxY2(tpc2Bounds.MaxY());
                const float driftMinZ2(tpc2Bounds.MinZ()), driftMaxZ2(tpc2Bounds.MaxZ());

                const double min2((0.5 * (driftMinX2 + driftMaxX2)) - 0.25 * std::fabs(driftMaxX2 - driftMinX2));
                const double max2((0.5 * (driftMinX2 + driftMaxX2)) + 0.25 * std::fabs(driftMaxX2 - driftMinX2));

                if ((min2 > max1) || (min1 > max2))
                    continue;

                driftMinX = std::min(driftMinX, driftMinX2);
                driftMaxX = std::max(driftMaxX, driftMaxX2);
                driftMinY = std::min(driftMinY, driftMinY2);
                driftMaxY = std::max(driftMaxY, driftMaxY2);
                driftMinZ = std::min(driftMinZ, driftMinZ2);
                driftMaxZ = std::max(driftMaxZ, driftMaxZ2);
            }

            BoxVolume driftVolume(
                {0.5f * (driftMinX + driftMaxX), 0.5f * (driftMinY + driftMaxY), 0.5f * (driftMinZ + driftMaxZ)},
                (driftMaxX - driftMinX), (driftMaxY - driftMinY), (driftMaxZ - driftMinZ));
            volumes.push_back(driftVolume);
        }
    }

    if (hepEVDVerboseLogging)
        std::cout << "HepEVD: Setting geometry with " << volumes.size() << " volumes." << std::endl;

    hepEVDServer = new HepEVDServer(DetectorGeometry(volumes));

    // Register the clear functions for the hit maps,
    // so we can clear them when we need to.
    HepEVD::registerClearFunction([&]() { recoHitToEvdHit.clear(); });
    HepEVD::registerClearFunction([&]() { spacePointToEvdHit.clear(); });
}

// Helper function to convert a geo::View_t to a HepEVD Hit Dimension.
static HitDimension getHepEVDHitDimension(geo::View_t geoHitType) {
    switch (geoHitType) {
    case geo::kU:
    case geo::kV:
    case geo::kW:
        return HitDimension::TWO_D;
    case geo::k3D:
        return HitDimension::THREE_D;
    default:
        // Default to 3D.
        // 2D hits have some special handling in HepEVD, so we don't want to
        // accidentally add them as 2D and get weird behaviour.
        return HitDimension::THREE_D;
    }
}

// Helper function to convert a geo::View_t to a HepEVD Hit Type.
static HitType getHepEVDHitType(geo::View_t geoHitType) {
    switch (geoHitType) {
    case geo::kU:
        return HitType::TWO_D_U;
    case geo::kV:
        return HitType::TWO_D_V;
    case geo::kW:
        return HitType::TWO_D_W;
    default:
        return HitType::GENERAL;
    }
}

static Hit *getHitFromRecobHit(const art::Ptr<recob::Hit> &hit) {
    const auto wireId(hit->WireID());
    const auto view(hit->View());

    const float x(hepEVDDetProps->ConvertTicksToX(hit->PeakTime(), wireId.Plane, wireId.TPC, wireId.Cryostat));
    const float e(hit->Integral());

    // Figure out the hits secondary coordinate.
    const auto wirePos(hepEvdLArSoftGeo->Wire(wireId).GetCenter());
    const float theta(0.5f * M_PI - hepEvdLArSoftGeo->WireAngleToVertical(view, wireId));
    const float z(wirePos.Z() * cos(theta) - wirePos.Y() * sin(theta));

    // Now we can make a HepEVD hit.
    Hit *hepEvdHit = new Hit({x, 0.0, z}, e);
    hepEvdHit->setDim(getHepEVDHitDimension(view));
    hepEvdHit->setHitType(getHepEVDHitType(view));

    return hepEvdHit;
}

static void addRecoHits(const art::Event &evt, const std::string hitLabel, const std::string label = "") {

    if (!isServerInitialised())
        return;

    // Get the hit and detector property info.
    auto const detProps = art::ServiceHandle<detinfo::DetectorPropertiesService const>()->DataFor(evt);
    hepEVDDetProps = &detProps;

    art::Handle<std::vector<recob::Hit>> hitHandle;
    std::vector<art::Ptr<recob::Hit>> hitVector;

    if (!evt.getByLabel(hitLabel, hitHandle)) {
        if (hepEVDVerboseLogging)
            std::cout << "HepEVD: Failed to get recob::Hit data product." << std::endl;
        throw cet::exception("HepEVD") << "Failed to get recob::Hit data product." << std::endl;
    }
    art::fill_ptr_vector(hitVector, hitHandle);

    if (hepEVDVerboseLogging)
        std::cout << "HepEVD: Processing " << hitVector.size() << " recob::Hit objects." << std::endl;

    Hits hits;

    for (const auto &hit : hitVector) {
        const auto hepEvdHit(getHitFromRecobHit(hit));
        hits.push_back(hepEvdHit);
        recoHitToEvdHit.insert({hit, hepEvdHit});
    }

    hepEVDServer->addHits(hits);
}

static void showMCParticles(const art::Event &evt, const std::string hitLabel, const std::string backTrackerLabel) {

    if (!isServerInitialised())
        return;

    // Get the hit and detector property info.
    auto const detProps = art::ServiceHandle<detinfo::DetectorPropertiesService const>()->DataFor(evt);
    hepEVDDetProps = &detProps;

    art::Handle<std::vector<recob::Hit>> hitHandle;
    std::vector<art::Ptr<recob::Hit>> hitVector;

    if (!evt.getByLabel(hitLabel, hitHandle)) {
        if (hepEVDVerboseLogging)
            std::cout << "HepEVD: Failed to get recob::Hit data product." << std::endl;
        throw cet::exception("HepEVD") << "Failed to get recob::Hit data product." << std::endl;
    }
    art::fill_ptr_vector(hitVector, hitHandle);

    // Get backtracker info
    const auto assocMCPart(
        art::FindManyP<simb::MCParticle, anab::BackTrackerHitMatchingData>(hitHandle, evt, backTrackerLabel));

    MCHits mcHits;

    // Truth match
    for (unsigned int hitIndex = 0; hitIndex < hitVector.size(); hitIndex++) {
        const art::Ptr<recob::Hit> &hit(hitVector[hitIndex]);
        const std::vector<art::Ptr<simb::MCParticle>> &matchedMCParticles(assocMCPart.at(hit.key()));
        auto matchedDatas(assocMCPart.data(hit.key()));

        for (unsigned int mcParticleIndex = 0; mcParticleIndex < matchedMCParticles.size(); ++mcParticleIndex) {
            auto matchedData(matchedDatas.at(mcParticleIndex));
            if (!matchedData->isMaxIDE)
                continue;
            const art::Ptr<simb::MCParticle> &mcParticle(matchedMCParticles.at(mcParticleIndex));

            const auto hepEvdHit(getHitFromRecobHit(hit));
            MCHit *mcHit = new MCHit(hepEvdHit->getPosition(), mcParticle->PdgCode(), hepEvdHit->getEnergy());
            mcHit->setDim(hepEvdHit->getDim());
            mcHit->setHitType(hepEvdHit->getHitType());
            mcHits.push_back(mcHit);
        }
    }

    hepEVDServer->addMCHits(mcHits);
}

static void showMCTruth(const art::Event &evt, const std::string mcTruthLabel) {

    if (!isServerInitialised())
        return;

    art::Handle<std::vector<simb::MCTruth>> mcTruthHandle;
    std::vector<art::Ptr<simb::MCTruth>> mcTruthVector;

    if (!evt.getByLabel(mcTruthLabel, mcTruthHandle)) {
        if (hepEVDVerboseLogging)
            std::cout << "HepEVD: Failed to get simb::MCTruth data product." << std::endl;
        throw cet::exception("HepEVD") << "Failed to get simb::MCTruth data product." << std::endl;
    }
    art::fill_ptr_vector(mcTruthVector, mcTruthHandle);

    // Now, we can build up a string to show the interaction as a string:
    //   - \nu_e (3.1 GeV) -> e- (0.51 GeV) + p ...
    std::string incomingParticles;
    std::string outgoingParticles;

    // TODO: Double check this...really we just want the direct result
    //       of the Nu interaction, not everything.
    for (unsigned int i = 0; i < mcTruthVector[0]->NParticles(); ++i) {
        const auto particle(mcTruthVector[0]->GetParticle(i));
        std::string pdgString;

        if (!pdgIsVisible(particle.PdgCode()))
            continue;
        else if (particle.P() > 0.05)
            pdgString = pdgToString(particle.PdgCode(), particle.P());
        else
            pdgString = pdgToString(particle.PdgCode());

        if (particle.StatusCode() == 0) {
            if (!incomingParticles.empty())
                incomingParticles += " + ";
            incomingParticles += pdgString;
        } else if (particle.StatusCode() == 1) {
            if (!outgoingParticles.empty())
                outgoingParticles += " + ";
            outgoingParticles += pdgString;
        }
    }

    const std::string mcTruthString(incomingParticles + " \\rightarrow " + outgoingParticles);

    hepEVDServer->setMCTruth(mcTruthString);
}

static Particle *addParticle(const art::Ptr<recob::PFParticle> &pfp, const art::Ptr<recob::PFParticle> &parentPfp,
                             const std::vector<art::Ptr<recob::SpacePoint>> &spacePoints,
                             const std::vector<art::Ptr<recob::Cluster>> &clusters,
                             const std::vector<art::Ptr<recob::Vertex>> &vertices,
                             const art::FindManyP<recob::Hit> &clusterHitAssoc, const std::string label = "") {

    Hits hits;

    // Add the 2D Hits first, which we need to get to via the clusters...
    for (const auto &cluster : clusters) {

        std::vector<art::Ptr<recob::Hit>> clusterHits(clusterHitAssoc.at(cluster.key()));

        for (const auto &hit : clusterHits) {
            const auto hepEvdHit(getHitFromRecobHit(hit));
            hits.push_back(hepEvdHit);
            recoHitToEvdHit.insert({hit, hepEvdHit});
        }
    }

    // Next, add the 3D hits...
    for (const auto &spacePoint : spacePoints) {

        const auto pos(spacePoint->XYZ());
        const float x(pos[0]);
        const float y(pos[1]);
        const float z(pos[2]);

        Hit *hepEvdHit = new Hit({x, y, z}, 0.f);
        hepEvdHit->setDim(HitDimension::THREE_D);

        hits.push_back(hepEvdHit);
        spacePointToEvdHit.insert({spacePoint, hepEvdHit});
    }

    std::string id(getUUID());
    Particle *particle = new Particle(hits, id, pfp->PdgCode() == 13 ? "Track-like" : "Shower-like");

    // Set the interaction type, based on the parent PFP.
    const auto pdgCode(std::abs(parentPfp->PdgCode()));
    const auto isNeutrino(pfp->IsPrimary() && (pdgCode == 12 || pdgCode == 14 || pdgCode == 16));

    if (isNeutrino)
        particle->setInteractionType(InteractionType::NEUTRINO);
    else
        particle->setInteractionType(InteractionType::COSMIC);

    Markers vertexMarkers;

    for (const auto &vertex : vertices) {
        const auto pos(vertex->position());
        const float x(pos.X());
        const float y(pos.Y());
        const float z(pos.Z());

        Point recoVertex({x, y, z});

        if (!isNeutrino)
            recoVertex.setColour("yellow");

        vertexMarkers.push_back(recoVertex);

        // TODO: 2D vertex markers.
    }

    particle->setVertices(vertexMarkers);

    return particle;
}

static void addPFPs(const art::Event &evt, const std::string pfpModuleLabel, const std::string label = "") {

    if (!isServerInitialised())
        return;

    // Get the hit and detector property info.
    // TODO: Consider adding a setup command, maybe one the builds on top of isServerInitialised.
    // It could setup some of these global variables and quit out if not available.
    auto const detProps = art::ServiceHandle<detinfo::DetectorPropertiesService const>()->DataFor(evt);
    hepEVDDetProps = &detProps;

    Particles particles;
    std::map<const art::Ptr<recob::PFParticle>, Particle *> pfpToParticleMap;

    art::Handle<std::vector<recob::PFParticle>> pfpHandle;
    std::vector<art::Ptr<recob::PFParticle>> particleVector;

    if (!evt.getByLabel(pfpModuleLabel, pfpHandle)) {
        if (hepEVDVerboseLogging)
            std::cout << "HepEVD: Failed to get recob::PFParticle data product." << std::endl;
        throw cet::exception("HepEVD") << "Failed to get recob::PFParticle data product." << std::endl;
    }
    art::fill_ptr_vector(particleVector, pfpHandle);

    // Also need the cluster information, to go via it to get the recob::Hit info.
    art::Handle<std::vector<recob::Cluster>> clusterHandle;

    if (!evt.getByLabel(pfpModuleLabel, clusterHandle)) {
        if (hepEVDVerboseLogging)
            std::cout << "HepEVD: Failed to get recob::Cluster data product." << std::endl;
        throw cet::exception("HepEVD") << "Failed to get recob::Cluster data product." << std::endl;
    }

    const auto pfpClusterAssoc(art::FindManyP<recob::Cluster>(pfpHandle, evt, pfpModuleLabel));
    const auto clusterHitAssoc(art::FindManyP<recob::Hit>(clusterHandle, evt, pfpModuleLabel));
    const auto spacePointAssoc(art::FindManyP<recob::SpacePoint>(pfpHandle, evt, pfpModuleLabel));
    const auto vertexAssoc(art::FindManyP<recob::Vertex>(pfpHandle, evt, pfpModuleLabel));

    // First, build up some useful information about the particles, and then we can
    // go through and add the particles, and then add the parent/child relationships.
    std::map<int, art::Ptr<recob::PFParticle>> pfpMap;
    std::map<int, std::vector<int>> parentToChildMap;
    std::map<int, int> childToParentMap;

    // Link every PFP to its ID.
    // We can then use that when we traverse the parent/child relationships.
    for (const auto &pfp : particleVector)
        pfpMap.insert({pfp->Self(), pfp});

    for (const auto &pfp : particleVector) {

        auto currentPfp(pfp);

        while (!currentPfp->IsPrimary()) {

            if (pfpMap.find(currentPfp->Parent()) == pfpMap.end()) {
                if (hepEVDVerboseLogging)
                    std::cout << "HepEVD: Failed to find parent PFP with ID " << currentPfp->Parent() << std::endl;
                break;
            }

            currentPfp = pfpMap[currentPfp->Parent()];
        }

        parentToChildMap[currentPfp->Self()].push_back(pfp->Self());
        childToParentMap.insert({pfp->Self(), currentPfp->Self()});
    }

    // Now, with all that information, we can go through and add the particles.
    for (const auto &pfp : particleVector) {

        // Grab all the required bits...
        const std::vector<art::Ptr<recob::SpacePoint>> &pfpSpacePoints(spacePointAssoc.at(pfp.key()));
        const std::vector<art::Ptr<recob::Cluster>> &clusters(pfpClusterAssoc.at(pfp.key()));
        const std::vector<art::Ptr<recob::Vertex>> &vertices(vertexAssoc.at(pfp.key()));
        const auto parentPfp(pfpMap[childToParentMap[pfp->Self()]]);

        const auto particle = addParticle(pfp, parentPfp, pfpSpacePoints, clusters, vertices, clusterHitAssoc, label);
        particles.push_back(particle);
        pfpToParticleMap.insert({pfp, particle});
    }

    // Finally, link up the parent/child relationships.
    for (const auto &parentChildPair : parentToChildMap) {

        const auto parentPfp(pfpMap[parentChildPair.first]);
        const auto parentParticle(pfpToParticleMap[parentPfp]);

        for (const auto &childId : parentChildPair.second) {

            if (childId == parentPfp->Self())
                continue;

            const auto childPfp(pfpMap[childId]);
            const auto childParticle(pfpToParticleMap[childPfp]);

            parentParticle->addChild(childParticle->getID());
            childParticle->setParentID(parentParticle->getID());
        }
    }

    hepEVDServer->addParticles(particles);
}

}; // namespace HepEVD

#endif // HEP_EVD_LARSOFT_HELPERS_H
