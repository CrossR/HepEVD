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

// LArContent Includes
#include "larpandoracontent/LArHelpers/LArMCParticleHelper.h"

// Local Includes
#include "geometry.h"
#include "hits.h"

namespace HepEVD {
namespace PandoraHelpers {

using HepHitMap = std::map<const pandora::CaloHit *, Hit *>;

DetectorGeometry getHepEVDGeometry(const pandora::GeometryManager *manager) {

    Volumes volumes;

    for (const auto &tpcIndexPair : manager->GetLArTPCMap()) {
        const auto &lartpc = *(tpcIndexPair.second);
        BoxVolume *larTPCVolume =
            new BoxVolume(Position({lartpc.GetCenterX(), lartpc.GetCenterY(), lartpc.GetCenterZ()}), lartpc.GetWidthX(),
                          lartpc.GetWidthY(), lartpc.GetWidthZ());
        volumes.push_back(larTPCVolume);
    }

    return DetectorGeometry(volumes);
}

HitType getHepEVDHitType(pandora::HitType pandoraHitType) {
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

Hits getHepEVD2DHits(const pandora::CaloHitList *caloHits, HepHitMap &pandoraToCaloMap, std::string label = "") {

    Hits hits;

    for (const pandora::CaloHit *const pCaloHit : *caloHits) {
        const auto pos = pCaloHit->GetPositionVector();
        Hit *hit =
            new Hit({pos.GetX(), pos.GetY(), pos.GetZ()}, pCaloHit->GetMipEquivalentEnergy(), pCaloHit->GetTime());

        if (label != "")
            hit->setLabel(label);

        hit->setDim(HitDimension::TWO_D);
        hit->setType(getHepEVDHitType(pCaloHit->GetHitType()));

        hits.push_back(hit);
        pandoraToCaloMap.insert({pCaloHit, hit});
    }

    return hits;
}

MCHits getHepEVDMCHits(const pandora::Algorithm &pAlgorithm, const pandora::CaloHitList *pCaloHitList) {

    MCHits mcHits;

    const pandora::MCParticleList *pMCParticleList(nullptr);
    try {
        PandoraContentApi::GetCurrentList(pAlgorithm, pMCParticleList);
    } catch (pandora::StatusCodeException &) {
        return mcHits;
    }

    LArMCParticleHelper::MCContributionMap mcToHitsMap;
    std::function<bool(const pandora::MCParticle *const)> getAll = [](const pandora::MCParticle *const) {
        return true;
    };
    LArMCParticleHelper::SelectReconstructableMCParticles(
        pMCParticleList, pCaloHitList, LArMCParticleHelper::PrimaryParameters(), getAll, mcToHitsMap);

    std::cout << "In: " << pCaloHitList->size() << "/" << pMCParticleList->size() << ", Out: " << mcToHitsMap.size()
              << std::endl;

    for (auto const &mcCaloHitListPair : mcToHitsMap) {

        const auto mcParticle = mcCaloHitListPair.first;
        const auto caloHitList = mcCaloHitListPair.second;

        for (auto const caloHit : caloHitList) {

            const auto pos = caloHit->GetPositionVector();
            MCHit *mcHit = new MCHit({pos.GetX(), pos.GetY(), pos.GetZ()}, mcParticle->GetParticleId(),
                                     caloHit->GetMipEquivalentEnergy(), caloHit->GetTime());

            mcHit->setDim(HitDimension::TWO_D);
            mcHit->setType(getHepEVDHitType(pCaloHit->GetHitType()));

            mcHits.push_back(mcHit);
        }
    }

    return mcHits;
}

}; // namespace PandoraHelpers
}; // namespace HepEVD

#endif // HEP_EVD_PANDORA_HELPERS_H