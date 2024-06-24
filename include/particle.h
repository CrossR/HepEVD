//
// Particles
//
// Represent a particle, and any associated children.

#ifndef HEP_EVD_HIERARCHIES_H
#define HEP_EVD_HIERARCHIES_H

#include "hits.h"
#include "marker.h"
#include "utils.h"

#include "extern/json.hpp"
using json = nlohmann::json;

#include <map>
#include <vector>

namespace HepEVD {

enum InteractionType { BEAM, COSMIC, NEUTRINO, OTHER };
NLOHMANN_JSON_SERIALIZE_ENUM(InteractionType,
                             {{BEAM, "Beam"}, {COSMIC, "Cosmic"}, {NEUTRINO, "Neutrino"}, {OTHER, "Other"}});

enum RenderType { PARTICLE, TRACK, SHOWER, OTHER };
NLOHMANN_JSON_SERIALIZE_ENUM(RenderType,
                             {{PARTICLE, "Particle"}, {TRACK, "Track"}, {SHOWER, "Shower"}, {OTHER, "Other"}});

// Represent a single particle in the event.
class Particle {
  public:
    Particle() : hits({}), label(""), id(""), parentID(""), childIDs({}) {}
    Particle(const Hits &hits, const std::string id = "", const std::string &label = "")
        : hits(hits), label(label), id(id), parentID(""), childIDs({}) {}

    double getEnergy() const {
        double energy = 0.0;
        for (const auto &hit : this->hits)
            energy += hit->getEnergy();
        return energy;
    }

    unsigned int getNHits() const { return this->hits.size(); }
    std::string getLabel() const { return this->label; }
    std::string getID() const { return this->id; }

    void setVertices(const Markers &vertices) {
        // Check that the markers are all Point-type.
        for (const auto &marker : vertices) {
            if (!std::holds_alternative<Point>(marker)) {
                throw std::invalid_argument("All markers must be of type Point");
            }
        }

        this->vertices = vertices;
    }
    Markers getVertices() const { return this->vertices; }

    void setParentID(const std::string parentID) { this->parentID = parentID; }

    void setChildIDs(const std::vector<std::string> &childIDs) { this->childIDs = childIDs; }
    void addChild(const std::string childID) { this->childIDs.push_back(childID); }

    void setPrimary(bool primary) { this->primary = primary; }
    bool getPrimary() const { return this->primary; }

    void setInteractionType(InteractionType interactionType) { this->interactionType = interactionType; }
    InteractionType getInteractionType() const { return this->interactionType; }

    void setRenderType(RenderType renderType) { this->renderType = renderType; }
    RenderType getRenderType() const { return this->renderType; }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Particle, hits, vertices, label, id, primary, interactionType, renderType, parentID,
                                   childIDs);

  private:
    Hits hits;
    Markers vertices;
    std::string label;
    std::string id;

    bool primary;
    InteractionType interactionType;

    // How to render the particle.
    // Default is PARTICLE, but can also be TRACK, SHOWER or OTHER.
    RenderType renderType = RenderType::PARTICLE;

    // Lets just assume that the IDs are enough, and the list of particles
    // is available by the consumer (i.e. the Web UI). Less contained (could
    // instead have pointers to the parent/children), but easier to manage and
    // easier to serialise.
    std::string parentID;
    std::vector<std::string> childIDs;
};
using Particles = std::vector<Particle *>;

inline static void to_json(json &j, const Particles &particles) {

    if (particles.size() == 0) {
        j = json::array();
        return;
    }

    for (const auto &particle : particles) {
        j.push_back(*particle);
    }
}

inline static void from_json(const json &j, Particles &particles) {
    for (const auto &particle : j) {
        particles.push_back(new Particle(particle));
    }
}

}; // namespace HepEVD

#endif // HEP_EVD_HIERARCHIES_H
