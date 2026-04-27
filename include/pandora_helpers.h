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
// Somethings have been moved around or don't exist.
#if __has_include("larpandoracontent/LArObjects/LArSlice.h")
#include "larpandoracontent/LArObjects/LArSlice.h"
typedef lar_content::SliceList SliceList;
#else
#include "larpandoracontent/LArControlFlow/SlicingAlgorithm.h"
typedef lar_content::SlicingAlgorithm::SliceList SliceList;
#endif

#if __has_include("larpandoradlcontent/LArHelpers/LArDLHelper.h") && __has_include(<ATen/ATen.h>) && \
     __has_include(<torch/script.h>)
#include <ATen/ATen.h>
#include <torch/script.h>
#endif

// Helpful typedefs
typedef std::unordered_map<const pandora::Cluster *, unsigned int> ClusterToSliceIndexMap;

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

    if (isServerInitialised(true))
        return;

    Volumes volumes;

    for (const auto &tpcIndexPair : manager->GetLArTPCMap()) {
        const auto &lartpc = *(tpcIndexPair.second);
        BoxVolume larTPCVolume({lartpc.GetCenterX(), lartpc.GetCenterY(), lartpc.GetCenterZ()}, lartpc.GetWidthX(),
                               lartpc.GetWidthY(), lartpc.GetWidthZ());
        volumes.push_back(larTPCVolume);
    }

    // If we've only got a single volume...we may be in a slice worker.
    // Lets also load in the detector gaps, and just add them as separate
    // volumes, so we can at least see where they are.
    //
    // TODO: Other gaps too?
    if (volumes.size() == 1) {
        for (const auto *const pGap : manager->GetDetectorGapList()) {
            const pandora::BoxGap *const pBoxGap(dynamic_cast<const pandora::BoxGap *>(pGap));

            // Safety check in case there are other gap types (e.g. LineGap)
            if (!pBoxGap)
                continue;

            const pandora::CartesianVector &vertex = pBoxGap->GetVertex();
            const pandora::CartesianVector &side1 = pBoxGap->GetSide1();
            const pandora::CartesianVector &side2 = pBoxGap->GetSide2();
            const pandora::CartesianVector &side3 = pBoxGap->GetSide3();

            // Center is vertex + half of each side vector
            const pandora::CartesianVector center = vertex + (side1 * 0.5f) + (side2 * 0.5f) + (side3 * 0.5f);

            // Widths are the magnitudes of the side vectors
            const float widthX = side1.GetMagnitude();
            const float widthY = side2.GetMagnitude();
            const float widthZ = side3.GetMagnitude();

            BoxVolume gapVolume({center.GetX(), center.GetY(), center.GetZ()}, widthX, widthY, widthZ);
            gapVolume.setOpacity(0.25);

            volumes.push_back(gapVolume);
        }
    }

    hepEVDLog("Setting HepEVD geometry: " + std::to_string(volumes.size()) + " volumes.");
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

static HepEVD::Hits getHits(const pandora::CaloHitList *caloHits, std::string label = "") {

    Hits hits;

    for (const pandora::CaloHit *const pCaloHit : *caloHits) {
        const auto pos = pCaloHit->GetPositionVector();
        Hit *hit = new Hit({pos.GetX(), pos.GetY(), pos.GetZ()}, pCaloHit->GetMipEquivalentEnergy());

        if (label != "")
            hit->setLabel(label);

        hit->setDim(getHepEVDHitDimension(pCaloHit->GetHitType()));
        hit->setHitType(getHepEVDHitType(pCaloHit->GetHitType()));

        if (pCaloHit->GetCellSize1() > 1)
            hit->setWidth("x", pCaloHit->GetCellSize1());

        hits.push_back(hit);
        caloHitToEvdHit.insert({pCaloHit, hit});
    }

    return hits;
}

// Add the given CaloHits to the server.
static void addHits(const pandora::CaloHitList *caloHits, std::string label = "") {
    if (!isServerInitialised())
        return;

    const auto hits = HepEVD::getHits(caloHits, label);
    hepEVDLog("Adding " + std::to_string(hits.size()) + " hits to the HepEVD server.");
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

    HepEVD::Particles particles;

    for (const pandora::Cluster *const pCluster : *clusters) {
        pandora::CaloHitList clusterCaloHits;
        HepEVD::getAllCaloHits(pCluster, clusterCaloHits);

        auto clusterParticle = new Particle(HepEVD::getHits(&clusterCaloHits, label), getUUID());
        particles.push_back(clusterParticle);
    }

    hepEVDLog("Adding " + std::to_string(particles.size()) + " clusters to the HepEVD server.");
    hepEVDServer->addParticles(particles);
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

static void addSlices(const SliceList *slices, const std::string label = "") {

    if (!isServerInitialised())
        return;

    HepEVD::Particles particles;

    for (unsigned int sliceNumber = 0; sliceNumber < slices->size(); ++sliceNumber) {
        const auto slice = (*slices)[sliceNumber];
        HepEVD::Hits sliceHits;

        auto uHits = HepEVD::getHits(&slice.m_caloHitListU, label);
        sliceHits.insert(sliceHits.end(), uHits.begin(), uHits.end());
        auto vHits = HepEVD::getHits(&slice.m_caloHitListV, label);
        sliceHits.insert(sliceHits.end(), vHits.begin(), vHits.end());
        auto wHits = HepEVD::getHits(&slice.m_caloHitListW, label);
        sliceHits.insert(sliceHits.end(), wHits.begin(), wHits.end());

        auto sliceParticle = new Particle(sliceHits, getUUID());
        particles.push_back(sliceParticle);
    }

    hepEVDLog("Adding " + std::to_string(slices->size()) + " slices to the HepEVD server.");
    hepEVDServer->addParticles(particles);
}

static void showMC(const pandora::Algorithm &pAlgorithm, const std::string &listName = "") {

    if (!isServerInitialised())
        return;

    MCHits mcHits;
    pandora::CaloHitList caloHitList;

    // Don't rely on "GetCurrentList" here...just load everything and use everything.
    // We can use 3D hits here too, since they shouldn't connect back unless there is truly
    // 3D MC available in our event.
    std::vector<std::string> caloHitListNames{"CaloHitListU", "CaloHitListV", "CaloHitListW", "CaloHitList3D"};
    for (const auto &hitListName : caloHitListNames) {
        const pandora::CaloHitList *pCurrentCaloHitList(nullptr);
        try {
            PandoraContentApi::GetList(pAlgorithm, hitListName, pCurrentCaloHitList);
            caloHitList.insert(caloHitList.end(), pCurrentCaloHitList->begin(), pCurrentCaloHitList->end());
        } catch (pandora::StatusCodeException &) {
            continue;
        }
    }

    // Check we actually have any hits.
    if (caloHitList.empty()) {
        hepEVDLog("No CaloHits to build MC from! Skipping...");
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

    // Since this is for vis...not for analysis, just get everything.
    lar_content::LArMCParticleHelper::PrimaryParameters primaryParams;
    primaryParams.m_minPrimaryGoodHits = 1;
    primaryParams.m_minHitsForGoodView = 1;
    primaryParams.m_minPrimaryGoodViews = 1;
    primaryParams.m_selectInputHits = false;
    primaryParams.m_maxPhotonPropagation = std::numeric_limits<float>::max();

    const pandora::CaloHitList *pCaloHitList{&caloHitList};
    lar_content::LArMCParticleHelper::SelectReconstructableMCParticles(pMCParticleList, pCaloHitList, primaryParams,
                                                                       getAll, mcToHitsMap);

    for (auto const &mcCaloHitListPair : mcToHitsMap) {

        const auto mcParticle = mcCaloHitListPair.first;
        const auto caloHitList = mcCaloHitListPair.second;

        for (auto const caloHit : caloHitList) {

            const auto pos = caloHit->GetPositionVector();
            MCHit *mcHit = new MCHit({pos.GetX(), pos.GetY(), pos.GetZ()}, mcParticle->GetParticleId(),
                                     caloHit->GetMipEquivalentEnergy());

            mcHit->setDim(getHepEVDHitDimension(caloHit->GetHitType()));
            mcHit->setHitType(getHepEVDHitType(caloHit->GetHitType()));

            mcHits.push_back(mcHit);
        }
    }

    hepEVDLog("Adding " + std::to_string(mcHits.size()) + " MC hits to the HepEVD server.");
    hepEVDServer->addMCHits(mcHits);

    // Now, build up a string to show the interaction as a string:
    //   - \nu_e (3.18 GeV) -> e- (0.51 GeV) + p ...
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

    // Check if there is a hanging " + " at the end, and remove it.
    // This can happen if we have a final particle that is invisible.
    if (mcTruth.substr(mcTruth.size() - 3) == " + ")
        mcTruth = mcTruth.substr(0, mcTruth.size() - 3);

    hepEVDLog("Set MC truth to " + mcTruth);
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

    std::string id(getUUID());
    Particle *particle = new Particle(hits, id, pPfo->GetParticleId() == 13 ? "Track-like" : "Shower-like");

    const auto parentPfo(lar_content::LArPfoHelper::GetParentPfo(pPfo));
    const bool isNeutrinoOrFinalState(lar_content::LArPfoHelper::IsNeutrino(pPfo) ||
                                      lar_content::LArPfoHelper::IsNeutrinoFinalState(pPfo));
    const bool isNeutrinoChild(lar_content::LArPfoHelper::IsNeutrino(parentPfo));

    if (isNeutrinoOrFinalState || isNeutrinoChild)
        particle->setInteractionType(InteractionType::NEUTRINO);
    else
        particle->setInteractionType(InteractionType::COSMIC);

    const pandora::Vertex *vertex(nullptr);

    try {
        vertex = lar_content::LArPfoHelper::GetVertex(pPfo);
    } catch (pandora::StatusCodeException &) {
        if (hepEVDVerboseLogging)
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

    for (const auto &parentChildPair : parentToChildMap) {

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

    hepEVDLog("Adding " + std::to_string(particles.size()) + " particles...");
    hepEVDServer->addParticles(particles);
}

#if __has_include("larpandoradlcontent/LArHelpers/LArDLHelper.h") && __has_include(<ATen/ATen.h>) && \
     __has_include(<torch/script.h>)
template <typename T> static void addDLTensorImage(const at::Tensor inputImageTensor, const std::string name) {

    if (!isServerInitialised())
        return;

    const auto imageTensor = inputImageTensor.clone().squeeze();

    if (imageTensor.dim() != 2) {
        std::cout << "HepEVD: Input image should be 2D!" << std::endl;
        std::cout << "HepEVD: Was instead " << imageTensor.dim() << "D" << std::endl;
        std::cout << "HepEVD: Maybe pre-process the image first? (Apply softmax etc)" << std::endl;
        return;
    }

    auto imageAccessor = imageTensor.accessor<T, 2>();
    const unsigned int height = imageAccessor.size(0);
    const unsigned int width = imageAccessor.size(1);

    std::vector<std::vector<float>> imageVector;
    for (unsigned int y = 0; y < height; ++y) {
        std::vector<float> row;
        for (unsigned int x = 0; x < width; ++x) {
            row.push_back(imageAccessor[y][x]);
        }
        imageVector.push_back(row);
    }

    MonochromeImage *image = new MonochromeImage(imageVector, name);

    hepEVDLog("Adding " + name + " image to the HepEVD server.");
    hepEVDServer->addImages({image});
}
#endif

}; // namespace HepEVD

#endif // HEP_EVD_PANDORA_HELPERS_H
