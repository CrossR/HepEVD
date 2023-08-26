//
// Particles
//
// Represent a particle, and any associated children.

#ifndef HEP_EVD_HIERARCHIES_H
#define HEP_EVD_HIERARCHIES_H

#include "hits.h"
#include "utils.h"

#include "extern/json.hpp"
using json = nlohmann::json;

#include <map>
#include <vector>

namespace HepEVD {

// Represent a single particle in the event.
class Particle {
  public:
    Particle() : hits({}), label(""), id(""), parentID(""), childIDs({}) {}
    Particle(const Hits &hits, const std::string id = 0, const std::string &label = "")
        : hits(hits), label(label), id(id), parentID(""), childIDs({}) {}

    double getEnergy() const {
        double energy = 0.0;
        for (const auto &hit : this->hits)
            energy += hit->getEnergy();
        return energy;
    }

    unsigned int getNHits() const { return this->hits.size(); }

    void setParentID(const std::string parentID) { this->parentID = parentID; }
    void setChildIDs(const std::vector<std::string> &childIDs) { this->childIDs = childIDs; }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Particle, hits, label, id, parentID, childIDs);

  private:
    Hits hits;
    std::string label;
    std::string id;

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
