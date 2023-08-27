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
#include "larpandoracontent/LArHelpers/LArClusterHelper.h"
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
static bool isServerInitialised() { return hepEVDServer != nullptr && hepEVDServer->isInitialised(); }

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

    hepEVDServer->addMCHits(mcHits);
}

static void addPFOs(const pandora::PfoList *pPfoList, const std::string parentID = "",
                    std::vector<std::string>* childIDs = nullptr) {

    if (!isServerInitialised())
        return;

    Particles particles;
    const pandora::ParticleFlowObject *targetPfo = nullptr;

    std::string id = getUUID();

    for (const pandora::ParticleFlowObject *const pPfo : *pPfoList) {

        Hits hits;

        for (const pandora::Cluster *const pCluster : pPfo->GetClusterList()) {

            pandora::CaloHitList clusterList;
            pCluster->GetOrderedCaloHitList().FillCaloHitList(clusterList);

            for (const pandora::CaloHit *const pCaloHit : clusterList) {
                const auto pos = pCaloHit->GetPositionVector();
                Hit *hit = new Hit({pos.GetX(), pos.GetY(), pos.GetZ()}, pCaloHit->GetMipEquivalentEnergy());

                hit->setDim(lar_content::LArClusterHelper::GetClusterHitType(pCluster) == pandora::HitType::TPC_3D
                                ? HitDimension::THREE_D
                                : HitDimension::TWO_D);

                if (hit->getDim() == HitDimension::TWO_D)
                    hit->setHitType(getHepEVDHitType(pCaloHit->GetHitType()));

                // if (pPfo->GetParticleId() == 13)
                //     hit->setLabel("Track-like");
                // else if (pPfo->GetParticleId() == 11)
                //     hit->setLabel("Shower-like"

                hits.push_back(hit);
            }
        }

        std::vector<std::string> currentChildIDs;
        std::string currentParentID = id;
        addPFOs(&(pPfo->GetDaughterPfoList()), currentParentID, &currentChildIDs);

        Particle *particle = new Particle(hits, id, pPfo->GetParticleId() == 13 ? "Track-like" : "Shower-like");

        // If we are in a recursive call, set the parent/child IDs.
        if (parentID != "" && childIDs != nullptr) {
            particle->setParentID(parentID);
            childIDs->push_back(id);
        }

        particle->setChildIDs(currentChildIDs);

        particle->setPrimary(pPfo->GetParentPfoList().empty());

        if (lar_content::LArPfoHelper::IsNeutrino(pPfo)) {
            particle->setInteractionType(InteractionType::NEUTRINO);
            targetPfo = pPfo;
        } else if (lar_content::LArPfoHelper::IsTestBeam(pPfo)) {
            particle->setInteractionType(InteractionType::BEAM);
            targetPfo = pPfo;
        } else
            particle->setInteractionType(InteractionType::COSMIC);

        particles.push_back(particle);

        id = getUUID();
    }

    hepEVDServer->addParticles(particles);
}

}; // namespace HepEVD

#endif // HEP_EVD_PANDORA_HELPERS_H
