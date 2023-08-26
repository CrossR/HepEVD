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
    Particle() : hits({}), label(""), id(-1), parentID(-1), childIDs({}) {}
    Particle(const Hits &hits, const unsigned int id = 0, const std::string &label = "")
        : hits(hits), label(label), id(id), parentID(-1), childIDs({}) {}

    double getEnergy() const {
        double energy = 0.0;
        for (const auto &hit : this->hits)
            energy += hit->getEnergy();
        return energy;
    }

    unsigned int getNHits() const { return this->hits.size(); }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Particle, hits, label, id, parentID, childIDs);

  private:
    Hits hits;
    std::string label;
    unsigned int id;

    // Lets just assume that the IDs are enough, and the list of particles
    // is available by the consumer (i.e. the Web UI). Less contained (could
    // instead have pointers to the parent/children), but easier to manage and
    // easier to serialise.
    unsigned int parentID;
    std::vector<unsigned int> childIDs;
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
