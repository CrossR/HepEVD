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

using HitProperties = std::map<std::tuple<std::string, PropertyType>, double>;

class Hit {
  public:
    Hit() : id(getUUID()), position(), energy(0.0) {}
    Hit(const Position &pos, double e = 0) : id(getUUID()), position(pos), energy(e) {}
    Hit(const PosArray &pos, double e = 0) : id(getUUID()), position(pos), energy(e) {}

    void setDim(const HitDimension &dim) { this->position.setDim(dim); }
    void setHitType(const HitType &hitType) { this->position.setHitType(hitType); }
    void setLabel(const std::string &str) { this->label = str; }
    void setEnergy(double e) { this->energy = e; }
    void setPosition(const Position &pos) { this->position = pos; }
    void setWidth(const std::string &axis, const float width) { this->width.setValue(axis, width); }

    std::string getId() const { return this->id; }
    Position getPosition() const { return this->position; }
    Position getWidth() const { return this->width; }
    double getEnergy() const { return this->energy; }
    HitDimension getDim() const { return this->position.dim; }
    HitType getHitType() const { return this->position.hitType; }

    // If no type is specified, the type is assumed to be numeric.
    void addProperties(std::map<std::string, double> props) {
        for (const auto &propValuePair : props)
            this->properties.insert({{propValuePair.first, PropertyType::NUMERIC}, propValuePair.second});

        return;
    }

    // Add hit properties with a specified type.
    void addProperties(HitProperties props) {
        for (const auto &propValuePair : props)
            this->properties.insert(propValuePair);

        return;
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Hit, id, position, energy, width, label, properties);

  protected:
    std::string id;
    Position position;
    Position width = Position({1.0, 1.0, 1.0});
    double energy;
    std::string label;
    HitProperties properties;
};
using Hits = std::vector<Hit *>;

inline static void to_json(json &j, const Hits &hits) {

    if (hits.size() == 0) {
        j = json::array();
        return;
    }

    for (const auto &hit : hits) {
        j.push_back(*hit);
    }
}

inline static void from_json(const json &j, Hits &hits) {
    for (const auto &hit : j) {
        hits.push_back(new Hit(hit));
    }
}

// Convenience constructor for MC hits.
class MCHit : public Hit {
  public:
    MCHit() : Hit() {}
    MCHit(const Position &pos, const double energy) : Hit(pos, energy) {}
    MCHit(const Position &pos, const double pdgCode, const double energy) : Hit(pos, energy) {
        this->addProperties({{"PDG", pdgCode}});
    }
    MCHit(const PosArray &pos, const double pdgCode, const double energy) : Hit(pos, energy) {
        this->addProperties({{"PDG", pdgCode}});
    }

    void setPDG(const double pdgCode) { this->addProperties({{"PDG", pdgCode}}); }
    double getPDG() const {
        if (this->properties.count({"PDG", PropertyType::NUMERIC}) == 0) {
            return 0.0;
        }

        return this->properties.at({"PDG", PropertyType::NUMERIC});
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MCHit, position, energy, label, properties);
};
using MCHits = std::vector<MCHit *>;

inline static void to_json(json &j, const MCHits &hits) {

    if (hits.size() == 0) {
        j = json::array();
        return;
    }

    for (const auto &hit : hits) {

        // Skip hits without a PDG code.
        // This is because technically you can initialize an MCHit without a PDG code.
        // But, if you don't have a PDG code, you're not really an MCHit.
        if (hit->getPDG() == 0.0)
            continue;

        j.push_back(*hit);
    }
}

inline static void from_json(const json &j, MCHits &hits) {
    for (const auto &hit : j) {
        hits.push_back(new MCHit(hit));
    }
}

}; // namespace HepEVD

#endif // HEP_EVD_HITS_H
