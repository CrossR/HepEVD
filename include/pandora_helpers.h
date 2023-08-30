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

// Local Includes
#include "geometry.h"
#include "hits.h"
#include "particle.h"
#include "server.h"
#include "utils.h"

namespace HepEVD {

using HepHitMap = std::map<const pandora::CaloHit *, Hit *>;

// Global Server Instance + HitMap
// This means we don't have to worry about setting it up,
// or awkwardness around using across multiple functions.
inline HepEVDServer *hepEVDServer;
inline HepHitMap caloHitToEvdHit;

static HepHitMap *getHitMap() { return &caloHitToEvdHit; }

static bool isServerInitialised() {
    const bool isInit(hepEVDServer != nullptr && hepEVDServer->isInitialised());

    if (!isInit) {
        std::cout << "HepEVD Server is not initialised!" << std::endl;
        std::cout << "Please call HepEVD::setHepEVDGeometry(this->GetPandora.GetGeometry()) or similar." << std::endl;
        std::cout << "This should be done before any other calls to the event display." << std::endl;
    }

    return isInit;
}

static void startServer() {
    if (isServerInitialised())
        hepEVDServer->startServer();
}

static void resetServer(const bool resetGeo = false) { hepEVDServer->resetServer(resetGeo); }

static void setHepEVDGeometry(const pandora::GeometryManager *manager) {

    Volumes volumes;

    for (const auto &tpcIndexPair : manager->GetLArTPCMap()) {
        const auto &lartpc = *(tpcIndexPair.second);
        BoxVolume larTPCVolume({lartpc.GetCenterX(), lartpc.GetCenterY(), lartpc.GetCenterZ()}, lartpc.GetWidthX(),
                               lartpc.GetWidthY(), lartpc.GetWidthZ());
        volumes.push_back(larTPCVolume);
    }

    hepEVDServer = new HepEVDServer(DetectorGeometry(volumes));
}

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

static void add2DHits(const pandora::CaloHitList *caloHits, std::string label = "") {

    if (!isServerInitialised())
        return;

    Hits hits;

    for (const pandora::CaloHit *const pCaloHit : *caloHits) {
        const auto pos = pCaloHit->GetPositionVector();
        Hit *hit = new Hit({pos.GetX(), pos.GetY(), pos.GetZ()}, pCaloHit->GetMipEquivalentEnergy());

        if (label != "")
            hit->setLabel(label);

        hit->setDim(HitDimension::TWO_D);
        hit->setHitType(getHepEVDHitType(pCaloHit->GetHitType()));

        hits.push_back(hit);
        caloHitToEvdHit.insert({pCaloHit, hit});
    }

    std::cout << "Adding " << hits.size() << " hits to HepEVD..." << std::endl;
    hepEVDServer->addHits(hits);
}

static void addMarkers(const Markers &markers) {
    if (!isServerInitialised())
        return;

    hepEVDServer->addMarkers(markers);
}

static void addMCHits(const pandora::Algorithm &pAlgorithm, const pandora::CaloHitList *pCaloHitList) {

    if (!isServerInitialised())
        return;

    MCHits mcHits;

    const pandora::MCParticleList *pMCParticleList(nullptr);
    try {
        PandoraContentApi::GetCurrentList(pAlgorithm, pMCParticleList);
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

    std::cout << "Adding " << mcHits.size() << " MC hits to HepEVD..." << std::endl;
    hepEVDServer->addMCHits(mcHits);
}

Particle *addParticle(const pandora::Pandora &pPandora, const pandora::ParticleFlowObject *pPfo) {

    Hits hits;
    pandora::CaloHitList caloHitList;

    // By default, this helper only returns the 2D hits.
    // We want the 3D hits too, so add them as well.
    lar_content::LArPfoHelper::GetAllCaloHits(pPfo, caloHitList);
    lar_content::LArPfoHelper::GetCaloHits(pPfo, pandora::HitType::TPC_3D, caloHitList);
    lar_content::LArPfoHelper::GetIsolatedCaloHits(pPfo, pandora::HitType::TPC_3D, caloHitList);

    for (const pandora::CaloHit *const pCaloHit : caloHitList) {
        const auto pos = pCaloHit->GetPositionVector();
        Hit *hit = new Hit({pos.GetX(), pos.GetY(), pos.GetZ()}, pCaloHit->GetMipEquivalentEnergy());

        hit->setDim(getHepEVDHitDimension(pCaloHit->GetHitType()));
        hit->setHitType(getHepEVDHitType(pCaloHit->GetHitType()));

        hits.push_back(hit);
    }

    std::string id = getUUID();
    Particle *particle = new Particle(hits, id, pPfo->GetParticleId() == 13 ? "Track-like" : "Shower-like");

    if (lar_content::LArPfoHelper::IsNeutrino(pPfo) || lar_content::LArPfoHelper::IsNeutrinoFinalState(pPfo))
        particle->setInteractionType(InteractionType::NEUTRINO);
    else if (lar_content::LArPfoHelper::IsTestBeam(pPfo) || lar_content::LArPfoHelper::IsTestBeamFinalState(pPfo))
        particle->setInteractionType(InteractionType::BEAM);
    else
        particle->setInteractionType(InteractionType::COSMIC);

    const pandora::Vertex *vertex = lar_content::LArPfoHelper::GetVertex(pPfo);

    Markers vertices;
    Point recoVertex3D({vertex->GetPosition().GetX(), vertex->GetPosition().GetY(), vertex->GetPosition().GetZ()});
    if (particle->getInteractionType() == InteractionType::COSMIC) recoVertex3D.setColour("yellow");
    vertices.push_back(recoVertex3D);

    auto views({pandora::TPC_VIEW_U, pandora::TPC_VIEW_V, pandora::TPC_VIEW_W});
    for (auto view : views) {
        const pandora::CartesianVector vertex2D =
            lar_content::LArGeometryHelper::ProjectPosition(pPandora, vertex->GetPosition(), view);
        Point recoVertex2D({vertex2D.GetX(), vertex2D.GetY(), vertex2D.GetZ()}, HitDimension::TWO_D,
                           getHepEVDHitType(view));
        if (particle->getInteractionType() == InteractionType::COSMIC) recoVertex2D.setColour("yellow");
        vertices.push_back(recoVertex2D);
    }

    particle->setVertices(vertices);

    return particle;
}

static void addPFOs(const pandora::Pandora &pPandora, const pandora::PfoList *pPfoList) {

    if (!isServerInitialised())
        return;

    if (pPfoList->empty())
        return;

    Particles particles;
    std::map<const pandora::ParticleFlowObject *, Particle *> pfoToParticleMap;
    const pandora::ParticleFlowObject *targetPfo = nullptr;

    // First, get a HepEVD::Particle for every Pandora::PFO.
    for (const pandora::ParticleFlowObject *const pPfo : *pPfoList) {
        const auto particle = addParticle(pPandora, pPfo);
        particles.push_back(particle);
        pfoToParticleMap.insert({pPfo, particle});
    }

    // Now, we can add the parent/child relationships.
    //
    // Its a little easier to do this in two steps, just to avoid
    // having to worry about the order of the PFOs in the list or any
    // double counting.
    for (const pandora::ParticleFlowObject *const pPfo : *pPfoList) {
        if (pPfo->GetNDaughterPfos() == 0)
            continue;

        for (const auto childPfo : pPfo->GetDaughterPfoList()) {

            const auto parent = pfoToParticleMap.at(pPfo);
            const auto child = pfoToParticleMap.at(childPfo);

            parent->addChild(child->getID());
            child->setParentID(parent->getID());

            // We will need the target PFO later, so lets store it now.
            if (parent->getInteractionType() == InteractionType::NEUTRINO)
                targetPfo = pPfo;
            if (parent->getInteractionType() == InteractionType::BEAM)
                targetPfo = pPfo;
        }
    }

    std::cout << "Adding " << particles.size() << " PFOs to HepEVD..." << std::endl;
    hepEVDServer->addParticles(particles);
}

}; // namespace HepEVD

#endif // HEP_EVD_PANDORA_HELPERS_H
