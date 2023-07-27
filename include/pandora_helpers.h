//
// Pandora Helpers
//
// High-level helpers, optionally included with Pandora.
// Given some Pandora objects, convert them into HepEVD ones.

#ifndef HEP_EVD_PANDORA_HELPERS_H
#define HEP_EVD_PANDORA_HELPERS_H

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

Hits getHepEVD2DHits(const pandora::CaloHitList *caloHits, HepHitMap &pandoraToCaloMap, std::string label = "") {

    Hits hits;

    for (const pandora::CaloHit *const pCaloHit : *caloHits) {
        const auto pos = pCaloHit->GetPositionVector();
        Hit *hit =
            new Hit({pos.GetX(), pos.GetY(), pos.GetZ()}, pCaloHit->GetMipEquivalentEnergy(), pCaloHit->GetTime());

        if (label != "")
            hit->setLabel(label);

        hit->setHitType(HitType::TWO_D);

        switch (pCaloHit->GetHitType()) {
        case pandora::HitType::TPC_VIEW_U:
            hit->setHitClass(HitClass::TWO_D_U);
            break;
        case pandora::HitType::TPC_VIEW_V:
            hit->setHitClass(HitClass::TWO_D_V);
            break;
        case pandora::HitType::TPC_VIEW_W:
            hit->setHitClass(HitClass::TWO_D_W);
            break;
        }

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

            mcHit->setHitType(HitType::TWO_D);

            switch (caloHit->GetHitType()) {
            case pandora::HitType::TPC_VIEW_U:
                mcHit->setHitClass(HitClass::TWO_D_U);
                break;
            case pandora::HitType::TPC_VIEW_V:
                mcHit->setHitClass(HitClass::TWO_D_V);
                break;
            case pandora::HitType::TPC_VIEW_W:
                mcHit->setHitClass(HitClass::TWO_D_W);
                break;
            }

            mcHits.push_back(mcHit);
        }
    }

    return mcHits;
}

}; // namespace PandoraHelpers
}; // namespace HepEVD

#endif // HEP_EVD_PANDORA_HELPERS_H
