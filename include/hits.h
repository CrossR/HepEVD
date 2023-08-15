//
// Hits
//
// A hit represents an actual energy deposition in the detector.
// That is, a position but also information on the sort of hit,
// and associated energy and timing information.
//
// The dimension of a hit can vary: 2D or 3D.
// Then, a given dimension of hit can have sub-classes: A specific 2D view etc.

#ifndef HEP_EVD_HITS_H
#define HEP_EVD_HITS_H

#include "utils.h"

#include "extern/json.hpp"
using json = nlohmann::json;

#include <map>
#include <ostream>
#include <string>
#include <vector>

namespace HepEVD {

class Hit {
  public:
    Hit() : position(), energy(0.0) {}
    Hit(const Position &pos, double e = 0) : position(pos), energy(e) {}
    Hit(const PosArray &pos, double e = 0) : position(pos), energy(e) {}

    void setDim(const HitDimension &dim) { this->position.setDim(dim); }
    void setType(const HitType &type) { this->position.setType(type); }
    void setLabel(const std::string &str) { this->label = str; }

    // TODO: This may want to be extensible. I.e. string -> double + other
    // properties (CATEGORIC, NUMERIC) etc;
    void addProperties(std::map<std::string, double> props) {
        for (const auto &propValuePair : props)
            this->properties.insert({propValuePair});

        return;
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Hit, position, energy, label, properties);

  protected:
    Position position;
    double energy;
    std::string label;
    std::map<std::string, double> properties;
};
using Hits = std::vector<Hit *>;

void to_json(json &j, const Hits &hits) {
    for (const auto &hit : hits) {
        j.push_back(*hit);
    }
}

void from_json(const json &j, Hits &hits) {
    for (const auto &hit : j) {
        hits.push_back(new Hit(hit));
    }
}

// Convenience constructor for MC hits.
class MCHit : public Hit {
  public:
    MCHit() : Hit() {}
    MCHit(const Position &pos, const double pdgCode, const double energy = 0) : Hit(pos, energy) {
        this->addProperties({{"PDG", pdgCode}});
    }
    MCHit(const PosArray &pos, const double pdgCode, const double energy = 0) : Hit(pos, energy) {
        this->addProperties({{"PDG", pdgCode}});
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MCHit, position, energy, label, properties);
};
using MCHits = std::vector<MCHit *>;

void to_json(json &j, const MCHits &hits) {
    for (const auto &hit : hits) {
        j.push_back(*hit);
    }
}

void from_json(const json &j, MCHits &hits) {
    for (const auto &hit : j) {
        hits.push_back(new MCHit(hit));
    }
}

}; // namespace HepEVD

#endif // HEP_EVD_HITS_H
