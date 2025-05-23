/**
 *  @file   larpandoracontent/LArMonitoring/VisualMonitoringAlgorithm.cc
 *
 *  @license This file is licensed under the GNU Public License (GPL) v3.0. license,
 *           in accordance with the base file being that same license.
 *           See the license in full over at https://github.com/PandoraPFA/LArContent/blob/master/LICENSE.
 *
 *  @brief  This is a modified version of the Pandora VisualMonitoringAlgorithm.
 *          Instead of loading the ROOT TEve event display, all the calls are instead
 *          replaced with the equivalent calls to the HepEVD library.
 *
 *          This should "just work" and be a drop-in replacement for the original
 *          VisualMonitoringAlgorithm.
 *
 *          However, you are likely to want to include additional
 *          <DisplayEvent>false</DisplayEvent> tags in your XML, as HepEVD
 *          is much more useful (in my opinion) when showing multiple event states.
 *          As such, if you instead only display the final state, you will only
 *          interrupt Pandora once, rather than N times
 *
 *          Full instructions for HepEVD can be found at:
 *          https://github.com/CrossR/HepEVD/wiki
 *
 *          The original Pandora VisualMonitoringAlgorithm can be found at:
 *          https://github.com/PandoraPFA/LArContent/
 *
 */

#include "Pandora/AlgorithmHeaders.h"

#include "larpandoracontent/LArMonitoring/VisualMonitoringAlgorithm.h"

using namespace pandora;

#define HEP_EVD_PANDORA_HELPERS 1
#include "hep_evd.h"

namespace lar_content {

VisualMonitoringAlgorithm::VisualMonitoringAlgorithm()
    : m_showCurrentMCParticles(false), m_showCurrentCaloHits(false), m_showCurrentTracks(false),
      m_showCurrentClusters(false), m_showCurrentPfos(false), m_showCurrentVertices(false), m_displayEvent(true),
      m_showDetector(false), m_detectorView("xz"), m_showOnlyAvailable(false), m_showAssociatedTracks(false),
      m_hitColors("iterate"), m_thresholdEnergy(-1.f), m_transparencyThresholdE(-1.f), m_energyScaleThresholdE(1.f),
      m_scalingFactor(1.f), m_showPfoVertices(true), m_showPfoHierarchy(true) {}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode VisualMonitoringAlgorithm::Run() {
    std::string stateName(m_caloHitListNames.size() == 0 ? "3D" : "2D");
    HepEVD::setHepEVDGeometry(this->GetPandora().GetGeometry());

    // Show current mc particles
    if (m_showCurrentMCParticles) {
        stateName = "CurrentMC";
        this->VisualizeMCParticleList(std::string());
    }

    // Show specified lists of mc particles
    for (StringVector::const_iterator iter = m_mcParticleListNames.begin(), iterEnd = m_mcParticleListNames.end();
         iter != iterEnd; ++iter) {
        stateName = "SelectedMC";
        this->VisualizeMCParticleList(*iter);
    }

    // Show current calo hit list
    if (m_showCurrentCaloHits) {
        stateName = "CurrentCaloHitLists";
        this->VisualizeCaloHitList(std::string());
    }

    // Show specified lists of calo hits
    for (StringVector::const_iterator iter = m_caloHitListNames.begin(), iterEnd = m_caloHitListNames.end();
         iter != iterEnd; ++iter) {
        stateName = "SelectedCaloHitLists";
        this->VisualizeCaloHitList(*iter);
    }

    // Show current cluster list
    if (m_showCurrentClusters) {
        stateName = "CurrentClusters";
        this->VisualizeClusterList(std::string());
    }

    // Show specified lists of clusters
    for (StringVector::const_iterator iter = m_clusterListNames.begin(), iterEnd = m_clusterListNames.end();
         iter != iterEnd; ++iter) {
        stateName = "SelectedClusters";
        this->VisualizeClusterList(*iter);
    }

    // Show current track list
    if (m_showCurrentTracks) {
        this->VisualizeTrackList(std::string());
    }

    // Show specified lists of tracks
    for (StringVector::const_iterator iter = m_trackListNames.begin(), iterEnd = m_trackListNames.end();
         iter != iterEnd; ++iter) {
        this->VisualizeTrackList(*iter);
    }

    // Show current particle flow objects
    if (m_showCurrentPfos) {
        stateName = "CurrentPfos";
        this->VisualizeParticleFlowList(std::string());
    }

    // Show specified lists of pfo
    for (StringVector::const_iterator iter = m_pfoListNames.begin(), iterEnd = m_pfoListNames.end(); iter != iterEnd;
         ++iter) {
        stateName = "SelectedPfos";
        this->VisualizeParticleFlowList(*iter);
    }

    // Show current vertex objects
    if (m_showCurrentVertices) {
        stateName = "CurrentVertices";
        this->VisualizeVertexList(std::string());
    }

    // Show specified lists of vertices
    for (StringVector::const_iterator iter = m_vertexListNames.begin(), iterEnd = m_vertexListNames.end();
         iter != iterEnd; ++iter) {
        stateName = "SelectedVertices";
        this->VisualizeVertexList(*iter);
    }

    // ...Do something stupid, and hijack the detector view variable to set the state name...
    // Not ideal, but it works and means you don't have to edit the .h file.
    if (m_detectorView != "xz")
        stateName = m_detectorView;

    // Lets also hijack the m_showAssociatedTracks variable, to store if we want to reset
    // the states on show.
    bool resetOnShow = m_showAssociatedTracks;

    // Finally, display the event and pause application
    if (m_displayEvent) {
        HepEVD::saveState(stateName, 1, resetOnShow);
    } else {
        HepEVD::saveState(stateName, 99, false);
    }

    return STATUS_CODE_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------------------------

void VisualMonitoringAlgorithm::VisualizeMCParticleList(const std::string &listName) const {
    HepEVD::showMC(*this, listName);
}

//------------------------------------------------------------------------------------------------------------------------------------------

void VisualMonitoringAlgorithm::VisualizeCaloHitList(const std::string &listName) const {
    const CaloHitList *pCaloHitList = NULL;

    if (listName.empty()) {
        if (STATUS_CODE_SUCCESS != PandoraContentApi::GetCurrentList(*this, pCaloHitList)) {
            if (PandoraContentApi::GetSettings(*this)->ShouldDisplayAlgorithmInfo())
                std::cout << "VisualMonitoringAlgorithm: current calo hit list unavailable." << std::endl;
            return;
        }
    } else {
        if (STATUS_CODE_SUCCESS != PandoraContentApi::GetList(*this, listName, pCaloHitList)) {
            if (PandoraContentApi::GetSettings(*this)->ShouldDisplayAlgorithmInfo())
                std::cout << "VisualMonitoringAlgorithm: calo hit list " << listName << " unavailable." << std::endl;
            return;
        }
    }

    // Filter calo hit list
    CaloHitList caloHitList;

    for (CaloHitList::const_iterator iter = pCaloHitList->begin(), iterEnd = pCaloHitList->end(); iter != iterEnd;
         ++iter) {
        const CaloHit *const pCaloHit = *iter;

        if ((pCaloHit->GetElectromagneticEnergy() > m_thresholdEnergy) &&
            (!m_showOnlyAvailable || PandoraContentApi::IsAvailable(*this, pCaloHit))) {
            caloHitList.push_back(pCaloHit);
        }
    }

    HepEVD::addHits(&caloHitList);
}

//------------------------------------------------------------------------------------------------------------------------------------------

void VisualMonitoringAlgorithm::VisualizeTrackList(const std::string & /*listName*/) const {
    std::cout << "VisualMonitoringAlgorithm: VisualizeTrackList not implemented in HepEVD yet!" << std::endl;
    return;
}

//------------------------------------------------------------------------------------------------------------------------------------------

void VisualMonitoringAlgorithm::VisualizeClusterList(const std::string &listName) const {
    const ClusterList *pClusterList = NULL;

    if (listName.empty()) {
        if (STATUS_CODE_SUCCESS != PandoraContentApi::GetCurrentList(*this, pClusterList)) {
            if (PandoraContentApi::GetSettings(*this)->ShouldDisplayAlgorithmInfo())
                std::cout << "VisualMonitoringAlgorithm: current cluster list unavailable." << std::endl;
            return;
        }
    } else {
        if (STATUS_CODE_SUCCESS != PandoraContentApi::GetList(*this, listName, pClusterList)) {
            if (PandoraContentApi::GetSettings(*this)->ShouldDisplayAlgorithmInfo())
                std::cout << "VisualMonitoringAlgorithm: cluster list " << listName << " unavailable." << std::endl;
            return;
        }
    }

    // Filter cluster list
    ClusterList clusterList;

    for (ClusterList::const_iterator iter = pClusterList->begin(), iterEnd = pClusterList->end(); iter != iterEnd;
         ++iter) {
        const Cluster *const pCluster = *iter;

        if (!m_showOnlyAvailable || PandoraContentApi::IsAvailable(*this, pCluster))
            clusterList.push_back(pCluster);
    }

    HepEVD::addClusters(&clusterList, listName == "" ? "DefaultClusterList" : listName);
}

//------------------------------------------------------------------------------------------------------------------------------------------

void VisualMonitoringAlgorithm::VisualizeParticleFlowList(const std::string &listName) const {
    const PfoList *pPfoList = NULL;

    if (listName.empty()) {
        if (STATUS_CODE_SUCCESS != PandoraContentApi::GetCurrentList(*this, pPfoList)) {
            if (PandoraContentApi::GetSettings(*this)->ShouldDisplayAlgorithmInfo())
                std::cout << "VisualMonitoringAlgorithm: current pfo list unavailable." << std::endl;
            return;
        }
    } else {
        if (STATUS_CODE_SUCCESS != PandoraContentApi::GetList(*this, listName, pPfoList)) {
            if (PandoraContentApi::GetSettings(*this)->ShouldDisplayAlgorithmInfo())
                std::cout << "VisualMonitoringAlgorithm: pfo list " << listName << " unavailable." << std::endl;
            return;
        }
    }

    HepEVD::addPFOs(this->GetPandora(), pPfoList);
}

//------------------------------------------------------------------------------------------------------------------------------------------

void VisualMonitoringAlgorithm::VisualizeVertexList(const std::string &listName) const {
    const VertexList *pVertexList = NULL;

    if (listName.empty()) {
        if (STATUS_CODE_SUCCESS != PandoraContentApi::GetCurrentList(*this, pVertexList)) {
            if (PandoraContentApi::GetSettings(*this)->ShouldDisplayAlgorithmInfo())
                std::cout << "VisualMonitoringAlgorithm: current vertex list unavailable." << std::endl;
            return;
        }
    } else {
        if (STATUS_CODE_SUCCESS != PandoraContentApi::GetList(*this, listName, pVertexList)) {
            if (PandoraContentApi::GetSettings(*this)->ShouldDisplayAlgorithmInfo())
                std::cout << "VisualMonitoringAlgorithm: vertex list " << listName << " unavailable." << std::endl;
            return;
        }
    }

    // Filter vertex list
    HepEVD::Markers vertexList;

    for (VertexList::const_iterator iter = pVertexList->begin(), iterEnd = pVertexList->end(); iter != iterEnd;
         ++iter) {
        const Vertex *const pVertex = *iter;

        if (m_showOnlyAvailable && pVertex->IsAvailable())
            continue;

        const auto pos = pVertex->GetPosition();
        HepEVD::Point vertexMarker({pos.GetX(), pos.GetY(), pos.GetZ()});

        if (pVertex->GetVertexType() == VertexType::VERTEX_U)
            vertexMarker.setHitType(HepEVD::TWO_D_U);
        if (pVertex->GetVertexType() == VertexType::VERTEX_V)
            vertexMarker.setHitType(HepEVD::TWO_D_V);
        if (pVertex->GetVertexType() == VertexType::VERTEX_W)
            vertexMarker.setHitType(HepEVD::TWO_D_W);

        if (pVertex->GetVertexType() != VertexType::VERTEX_3D)
            vertexMarker.setDim(HepEVD::TWO_D);
        else
            vertexMarker.setDim(HepEVD::THREE_D);
        vertexList.push_back(vertexMarker);
    }

    HepEVD::addMarkers(vertexList);
}

//------------------------------------------------------------------------------------------------------------------------------------------

StatusCode VisualMonitoringAlgorithm::ReadSettings(const TiXmlHandle xmlHandle) {
    PANDORA_RETURN_RESULT_IF_AND_IF(
        STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
        XmlHelper::ReadValue(xmlHandle, "ShowCurrentMCParticles", m_showCurrentMCParticles));

    PANDORA_RETURN_RESULT_IF_AND_IF(
        STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
        XmlHelper::ReadVectorOfValues(xmlHandle, "MCParticleListNames", m_mcParticleListNames));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
                                    XmlHelper::ReadValue(xmlHandle, "ShowCurrentCaloHits", m_showCurrentCaloHits));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
                                    XmlHelper::ReadVectorOfValues(xmlHandle, "CaloHitListNames", m_caloHitListNames));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
                                    XmlHelper::ReadValue(xmlHandle, "ShowCurrentClusters", m_showCurrentClusters));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
                                    XmlHelper::ReadVectorOfValues(xmlHandle, "ClusterListNames", m_clusterListNames));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
                                    XmlHelper::ReadValue(xmlHandle, "ShowCurrentPfos", m_showCurrentPfos));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
                                    XmlHelper::ReadVectorOfValues(xmlHandle, "PfoListNames", m_pfoListNames));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
                                    XmlHelper::ReadValue(xmlHandle, "ShowCurrentVertices", m_showCurrentVertices));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
                                    XmlHelper::ReadVectorOfValues(xmlHandle, "VertexListNames", m_vertexListNames));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
                                    XmlHelper::ReadValue(xmlHandle, "DisplayEvent", m_displayEvent));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
                                    XmlHelper::ReadValue(xmlHandle, "ShowOnlyAvailable", m_showOnlyAvailable));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
                                    XmlHelper::ReadValue(xmlHandle, "ThresholdEnergy", m_thresholdEnergy));

    StringVector stateName;
    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
                                    XmlHelper::ReadVectorOfValues(xmlHandle, "StateName", stateName));

    PANDORA_RETURN_RESULT_IF_AND_IF(STATUS_CODE_SUCCESS, STATUS_CODE_NOT_FOUND, !=,
                                    XmlHelper::ReadValue(xmlHandle, "ResetStateOnShow", m_showAssociatedTracks));

    // Hijack the detector view variable to store the state name.
    if (stateName.size() > 0) {
        m_detectorView = stateName[0] + " ";
        for (unsigned int i = 1; i < stateName.size(); ++i)
            m_detectorView += stateName[i] + " ";

        m_detectorView.pop_back();
    }

// If we don't use the other private fields, some compilers will complain about unused variables.
// This is a hack to silence them.
#define UNUSED(x) (void)(x)
    UNUSED(m_showDetector);
    UNUSED(m_transparencyThresholdE);
    UNUSED(m_energyScaleThresholdE);
    UNUSED(m_scalingFactor);
    UNUSED(m_showPfoVertices);
    UNUSED(m_showPfoHierarchy);

    return STATUS_CODE_SUCCESS;
}

} // namespace lar_content
