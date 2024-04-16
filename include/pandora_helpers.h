//
// Pandora Helpers
//
// High-level helpers, optionally included with Pandora.
// Given some Pandora objects, convert them into HepEVD ones.

#ifndef HEP_EVD_PANDORA_HELPERS_H
#define HEP_EVD_PANDORA_HELPERS_H

#include <map>
#include <string>

// SDK Includes
#include "Geometry/LArTPC.h"
#include "Managers/GeometryManager.h"
#include "Objects/CaloHit.h"
#include "Pandora/AlgorithmHeaders.h"

// LArContent Includes
#include "larpandoracontent/LArHelpers/LArGeometryHelper.h"
#include "larpandoracontent/LArHelpers/LArMCParticleHelper.h"
#include "larpandoracontent/LArHelpers/LArPfoHelper.h"

// Backwards compatibility with older versions of Pandora.
// Somethings have been moved around, so we need to check for them.
#ifdef LAR_SLICE_H
#include "larpandoracontent/LArObjects/LArSlice.h"
typedef lar_content::LArSlice::SliceList SliceList;
#else
#include "larpandoracontent/LArControlFlow/SlicingAlgorithm.h"
typedef lar_content::SlicingAlgorithm::SliceList SliceList;
#endif

// Local Includes
#include "base_helper.h"
#include "geometry.h"
#include "hits.h"
#include "particle.h"
#include "server.h"
#include "utils.h"

namespace HepEVD {

using PandoraHitMap = std::map<const pandora::CaloHit *, Hit *>;
inline PandoraHitMap caloHitToEvdHit;

// Get the current hit map, such that properties and more can be added
// to the HepEVD hits.
static PandoraHitMap *getHitMap() { return &caloHitToEvdHit; }

// Set the HepEVD geometry by pulling the relevant information from the
// Pandora GeometryManager.
// Also register the map to the manager, so we don't leak memory.
static void setHepEVDGeometry(const pandora::GeometryManager *manager) {

    if (isServerInitialised())
        return;

    Volumes volumes;

    for (const auto &tpcIndexPair : manager->GetLArTPCMap()) {
        const auto &lartpc = *(tpcIndexPair.second);
        BoxVolume larTPCVolume({lartpc.GetCenterX(), lartpc.GetCenterY(), lartpc.GetCenterZ()}, lartpc.GetWidthX(),
                               lartpc.GetWidthY(), lartpc.GetWidthZ());
        volumes.push_back(larTPCVolume);
    }

    hepEVDServer = new HepEVDServer(DetectorGeometry(volumes));

    // Register the clear function for the hit map,
    // so we can clear it when we need to.
    HepEVD::registerClearFunction([&]() { caloHitToEvdHit.clear(); });
}

// Helper function to convert a Pandora HitType to a HepEVD Hit Dimension.
static HitDimension getHepEVDHitDimension(pandora::HitType pandoraHitType) {
    switch (pandoraHitType) {
    case pandora::HitType::TPC_VIEW_U:
    case pandora::HitType::TPC_VIEW_V:
    case pandora::HitType::TPC_VIEW_W:
        return HitDimension::TWO_D;
    case pandora::HitType::TPC_3D:
        return HitDimension::THREE_D;
    default:
        // Default to 3D.
        // 2D hits have some special handling in HepEVD, so we don't want to
        // accidentally add them as 2D and get weird behaviour.
        return HitDimension::THREE_D;
    }
}

// Helper function to convert a Pandora HitType to a HepEVD Hit Type.
static HitType getHepEVDHitType(pandora::HitType pandoraHitType) {
    switch (pandoraHitType) {
    case pandora::HitType::TPC_VIEW_U:
        return HitType::TWO_D_U;
    case pandora::HitType::TPC_VIEW_V:
        return HitType::TWO_D_V;
    case pandora::HitType::TPC_VIEW_W:
        return HitType::TWO_D_W;
    default:
        return HitType::GENERAL;
    }
}

// Add the given CaloHits to the server.
static void addHits(const pandora::CaloHitList *caloHits, std::string label = "") {

    if (!isServerInitialised())
        return;

    Hits hits;

    for (const pandora::CaloHit *const pCaloHit : *caloHits) {
        const auto pos = pCaloHit->GetPositionVector();
        Hit *hit = new Hit({pos.GetX(), pos.GetY(), pos.GetZ()}, pCaloHit->GetMipEquivalentEnergy());

        if (label != "")
            hit->setLabel(label);

        hit->setDim(getHepEVDHitDimension(pCaloHit->GetHitType()));
        hit->setHitType(getHepEVDHitType(pCaloHit->GetHitType()));

        hits.push_back(hit);
        caloHitToEvdHit.insert({pCaloHit, hit});
    }

    hepEVDServer->addHits(hits);
}

static void getAllCaloHits(const pandora::Cluster *pCluster, pandora::CaloHitList &caloHitList) {
    pCluster->GetOrderedCaloHitList().FillCaloHitList(caloHitList);
    caloHitList.insert(caloHitList.end(), pCluster->GetIsolatedCaloHitList().begin(),
                       pCluster->GetIsolatedCaloHitList().end());
}

static void addClusters(const pandora::ClusterList *clusters, std::string label = "") {

    if (!isServerInitialised())
        return;

    unsigned int clusterNumber = 0;

    for (const pandora::Cluster *const pCluster : *clusters) {
        pandora::CaloHitList clusterCaloHits;
        HepEVD::getAllCaloHits(pCluster, clusterCaloHits);
        HepEVD::addHits(&clusterCaloHits, label);

        for (const auto &pCaloHit : clusterCaloHits) {

            if (caloHitToEvdHit.count(pCaloHit) == 0)
                continue;

            caloHitToEvdHit[pCaloHit]->addProperties(
                {{{"ClusterNumber", HepEVD::PropertyType::CATEGORIC}, clusterNumber}});
        }

        ++clusterNumber;
    }
}

static void addClusterProperties(const pandora::Cluster *cluster, std::map<std::string, double> props) {

    if (!isServerInitialised())
        return;

    for (const auto &orderedList : cluster->GetOrderedCaloHitList()) {
        for (const auto caloHit : *(orderedList.second)) {
            if (caloHitToEvdHit.count(caloHit) == 0)
                continue;

            caloHitToEvdHit[caloHit]->addProperties(props);
        }
    }
}

static void addSlices(const SliceList *slices, std::string label = "") {

    if (!isServerInitialised())
        return;

    for (unsigned int sliceNumber = 0; sliceNumber < slices->size(); ++sliceNumber) {
        const auto slice = (*slices)[sliceNumber];

        HepEVD::addHits(&slice.m_caloHitListU, label);
        HepEVD::addHits(&slice.m_caloHitListV, label);
        HepEVD::addHits(&slice.m_caloHitListW, label);

        for (const auto &pCaloHit : slice.m_caloHitListU)
            caloHitToEvdHit[pCaloHit]->addProperties({{{"SliceNumber", HepEVD::PropertyType::CATEGORIC}, sliceNumber}});
        for (const auto &pCaloHit : slice.m_caloHitListV)
            caloHitToEvdHit[pCaloHit]->addProperties({{{"SliceNumber", HepEVD::PropertyType::CATEGORIC}, sliceNumber}});
        for (const auto &pCaloHit : slice.m_caloHitListW)
            caloHitToEvdHit[pCaloHit]->addProperties({{{"SliceNumber", HepEVD::PropertyType::CATEGORIC}, sliceNumber}});
    }
}

static void showMC(const pandora::Algorithm &pAlgorithm, const std::string &listName = "") {

    if (!isServerInitialised())
        return;

    MCHits mcHits;

    const pandora::CaloHitList *pCaloHitList(nullptr);
    try {
        PandoraContentApi::GetCurrentList(pAlgorithm, pCaloHitList);
    } catch (pandora::StatusCodeException &) {
        return;
    }

    const pandora::MCParticleList *pMCParticleList(nullptr);
    try {
        if (listName.empty())
            PandoraContentApi::GetCurrentList(pAlgorithm, pMCParticleList);
        else
            PandoraContentApi::GetList(pAlgorithm, listName, pMCParticleList);
    } catch (pandora::StatusCodeException &) {
        return;
    }

    lar_content::LArMCParticleHelper::MCContributionMap mcToHitsMap;
    std::function<bool(const pandora::MCParticle *const)> getAll = [](const pandora::MCParticle *const) {
        return true;
    };
    lar_content::LArMCParticleHelper::SelectReconstructableMCParticles(
        pMCParticleList, pCaloHitList, lar_content::LArMCParticleHelper::PrimaryParameters(), getAll, mcToHitsMap);

    for (auto const &mcCaloHitListPair : mcToHitsMap) {

        const auto mcParticle = mcCaloHitListPair.first;
        const auto caloHitList = mcCaloHitListPair.second;

        for (auto const caloHit : caloHitList) {

            const auto pos = caloHit->GetPositionVector();
            MCHit *mcHit = new MCHit({pos.GetX(), pos.GetY(), pos.GetZ()}, mcParticle->GetParticleId(),
                                     caloHit->GetMipEquivalentEnergy());

            mcHit->setDim(HitDimension::TWO_D);
            mcHit->setHitType(getHepEVDHitType(caloHit->GetHitType()));

            mcHits.push_back(mcHit);
        }
    }

    hepEVDServer->addMCHits(mcHits);

    // Now, build up a string to show the interaction as a string:
    //   - \nu_e (3.18 GeV) -> e- (0.51 GeV) + n
    std::string mcTruth;

    pandora::MCParticleVector primaryMCParticles;
    lar_content::LArMCParticleHelper::GetPrimaryMCParticleList(pMCParticleList, primaryMCParticles);
    const pandora::MCParticle *pTrueNeutrino(nullptr);

    if (primaryMCParticles.empty())
        return;

    for (const auto pMCParticle : primaryMCParticles) {
        const pandora::MCParticleList &parents{pMCParticle->GetParentList()};
        if (parents.size() == 1 && lar_content::LArMCParticleHelper::IsNeutrino(parents.front())) {
            pTrueNeutrino = parents.front();
            break;
        }
    }

    if (pTrueNeutrino == nullptr)
        return;

    mcTruth += pdgToString(pTrueNeutrino->GetParticleId(), pTrueNeutrino->GetEnergy());
    mcTruth += " \\rightarrow ";

    // Finally, add in the primary children.
    const auto childParticles = pTrueNeutrino->GetDaughterList();
    for (const auto pMCParticle : childParticles) {

        if (pdgIsVisible(pMCParticle->GetParticleId()))
            mcTruth += pdgToString(pMCParticle->GetParticleId(), pMCParticle->GetEnergy());
        else
            continue;

        if (pMCParticle != childParticles.back())
            mcTruth += " + ";
    }

    hepEVDServer->setMCTruth(mcTruth);
}

// Helper function, as the "GetAllCaloHits" function isn't in some older versions of Pandora.
static void getAllCaloHits(const pandora::ParticleFlowObject *pPfo, pandora::CaloHitList &caloHitList) {

    std::vector<pandora::HitType> views({pandora::HitType::TPC_VIEW_U, pandora::HitType::TPC_VIEW_V,
                                         pandora::HitType::TPC_VIEW_W, pandora::HitType::TPC_3D});

    for (auto view : views) {
        lar_content::LArPfoHelper::GetCaloHits(pPfo, view, caloHitList);
        lar_content::LArPfoHelper::GetIsolatedCaloHits(pPfo, view, caloHitList);
    }
}

static Particle *addParticle(const pandora::Pandora &pPandora, const pandora::ParticleFlowObject *pPfo,
                             std::string label = "") {

    Hits hits;
    pandora::CaloHitList caloHitList;
    HepEVD::getAllCaloHits(pPfo, caloHitList);

    for (const pandora::CaloHit *const pCaloHit : caloHitList) {
        const auto pos = pCaloHit->GetPositionVector();
        Hit *hit = new Hit({pos.GetX(), pos.GetY(), pos.GetZ()}, pCaloHit->GetMipEquivalentEnergy());

        if (label != "")
            hit->setLabel(label);

        hit->setDim(getHepEVDHitDimension(pCaloHit->GetHitType()));
        hit->setHitType(getHepEVDHitType(pCaloHit->GetHitType()));

        hits.push_back(hit);
        caloHitToEvdHit.insert({pCaloHit, hit});
    }

    std::string id = getUUID();
    Particle *particle = new Particle(hits, id, pPfo->GetParticleId() == 13 ? "Track-like" : "Shower-like");

    if (lar_content::LArPfoHelper::IsNeutrino(pPfo) || lar_content::LArPfoHelper::IsNeutrinoFinalState(pPfo))
        particle->setInteractionType(InteractionType::NEUTRINO);
    else
        particle->setInteractionType(InteractionType::COSMIC);

    const pandora::Vertex *vertex(nullptr);

    try {
        vertex = lar_content::LArPfoHelper::GetVertex(pPfo);
    } catch (pandora::StatusCodeException &) {
        if (verboseLogging)
            std::cout << "HepEVD: Failed to get vertex for PFO!" << std::endl;
        return particle;
    }

    Markers vertices;
    Point recoVertex3D({vertex->GetPosition().GetX(), vertex->GetPosition().GetY(), vertex->GetPosition().GetZ()});
    if (particle->getInteractionType() == InteractionType::COSMIC)
        recoVertex3D.setColour("yellow");
    vertices.push_back(recoVertex3D);

    std::vector<pandora::HitType> views(
        {pandora::HitType::TPC_VIEW_U, pandora::HitType::TPC_VIEW_V, pandora::HitType::TPC_VIEW_W});
    for (auto view : views) {
        const pandora::CartesianVector vertex2D =
            lar_content::LArGeometryHelper::ProjectPosition(pPandora, vertex->GetPosition(), view);
        Point recoVertex2D({vertex2D.GetX(), vertex2D.GetY(), vertex2D.GetZ()}, HitDimension::TWO_D,
                           getHepEVDHitType(view));
        if (particle->getInteractionType() == InteractionType::COSMIC)
            recoVertex2D.setColour("yellow");
        vertices.push_back(recoVertex2D);
    }

    particle->setVertices(vertices);

    return particle;
}

static void addPFOs(const pandora::Pandora &pPandora, const pandora::PfoList *pPfoList, std::string label = "") {

    if (!isServerInitialised())
        return;

    if (pPfoList->empty())
        return;

    Particles particles;
    std::map<const pandora::ParticleFlowObject *, Particle *> pfoToParticleMap;

    // First, get a HepEVD::Particle for every Pandora::PFO.
    for (const pandora::ParticleFlowObject *const pPfo : *pPfoList) {
        const auto particle = addParticle(pPandora, pPfo, label);
        particles.push_back(particle);
        pfoToParticleMap.insert({pPfo, particle});
    }

    // Now, we can add the parent/child relationships.
    //
    // Its a little easier to do this in two steps, just to avoid
    // having to worry about the order of the PFOs in the list or any
    // double counting.
    std::map<const pandora::ParticleFlowObject *, const pandora::PfoList> parentToChildMap;
    for (const pandora::ParticleFlowObject *const pPfo : *pPfoList) {
        const auto parentPfo = lar_content::LArPfoHelper::GetParentPfo(pPfo);

        if (parentPfo == nullptr || parentToChildMap.count(parentPfo) == 1)
            continue;

        pandora::PfoList allChildren;
        lar_content::LArPfoHelper::GetAllDownstreamPfos(pPfo, allChildren);
        parentToChildMap.insert({parentPfo, allChildren});
    }

    for (const auto parentChildPair : parentToChildMap) {

        if (parentChildPair.second.empty() || pfoToParticleMap.count(parentChildPair.first) == 0)
            continue;

        const auto pPfo = parentChildPair.first;
        const auto parent = pfoToParticleMap.at(pPfo);

        for (const auto childPfo : parentChildPair.second) {

            // If any particles are missing from the original top level list, add them now.
            if (pfoToParticleMap.count(childPfo) == 0) {
                const auto particle = addParticle(pPandora, childPfo, label);
                particles.push_back(particle);
                pfoToParticleMap.insert({childPfo, particle});
            }

            if (pPfo == childPfo)
                continue;

            const auto child = pfoToParticleMap.at(childPfo);

            parent->addChild(child->getID());
            child->setParentID(parent->getID());
        }
    }

    hepEVDServer->addParticles(particles);
}

#ifdef LAR_GRAPH_H
static void addGraph(const lar_content::LArGraph &graph, std::string label = "", std::string nodeColour = "grey",
                     std::string lineColour = "blue") {

    Markers markers;
    const auto &edges = graph.GetEdges();

    for (const auto &edge : edges) {
        const auto &startHit = edge->m_v0;
        const auto &endHit = edge->m_v1;

        const auto &startPos = startHit->GetPositionVector();
        const auto &endPos = endHit->GetPositionVector();

        // Assume the two hits are in the same dimension / hit type.
        const auto hitType = getHepEVDHitType(startHit->GetHitType());
        const auto hitDim = getHepEVDHitDimension(startHit->GetHitType());

        Point startPoint({startPos.GetX(), startPos.GetY(), startPos.GetZ()}, hitDim, hitType);
        startPoint.setLabel(label);
        startPoint.setColour(nodeColour);

        Point endPoint({endPos.GetX(), endPos.GetY(), endPos.GetZ()}, hitDim, hitType);
        endPoint.setLabel(label);
        endPoint.setColour(nodeColour);

        Line line(startPoint, endPoint);
        line.setLabel(label);
        line.setColour(lineColour);

        markers.push_back(startPoint);
        markers.push_back(endPoint);
        markers.push_back(line);
    }

    hepEVDServer->addMarkers(markers);
}
#endif

}; // namespace HepEVD

#endif // HEP_EVD_PANDORA_HELPERS_H
