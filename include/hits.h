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

#include <map>
#include <ostream>
#include <string>
#include <vector>

namespace HepEVD {

enum HitDimension { THREE_D, TWO_D };
enum HitType { GENERAL, TWO_D_U, TWO_D_V, TWO_D_W };

class Hit {
  public:
    Hit(const Position &pos, double e = 0, double t = 0) : position(pos), time(t), energy(e) {}
    Hit(const std::array<double, 3> &pos, double e = 0, double t = 0) : position(pos), time(t), energy(e) {}

    void setDim(const HitDimension &dim) { this->dim = dim; }
    void setType(const HitType &type) { this->type = type; }
    void setLabel(const std::string &str) { this->label = str; }

    // TODO: This may want to be extensible. I.e. string -> double + other
    // properties (CATEGORIC, NUMERIC) etc;
    void addProperties(std::map<std::string, double> props) {
        for (const auto &propValuePair : props)
            this->properties.insert({propValuePair});

        return;
    }

    friend std::ostream &operator<<(std::ostream &os, Hit const &hit) {

        Position hitPosition(hit.position);

        // If this is a 2D hit, its easier in the EVD to consider XY, not XZ.
        if (hit.dim == TWO_D) {
            hitPosition.y = hit.position.z;
            hitPosition.z = 0;
        }

        os << "{"
           << "\"dim\": \"" << Hit::hitDimToString(hit.dim) << "\","
           << "\"type\": \"" << Hit::hitTypeToString(hit.type) << "\"," << hitPosition << ","
           << "\"time\": " << hit.time << ","
           << "\"energy\": " << hit.energy;

        if (hit.label != "")
            os << ", \"label\": \"" << hit.label << "\"";

        if (hit.properties.size() != 0) {
            os << ", \"properties\": {";
            for (const auto &propValuePair : hit.properties)
                os << "\"" << propValuePair.first << "\": " << propValuePair.second << ",";

            os.seekp(-1, os.cur);
            os << "}";
        }
        os << "}";

        return os;
    }

    static std::string hitDimToString(const HitDimension &hit) {
        switch (hit) {
        case THREE_D:
            return "3D";
        case TWO_D:
            return "2D";
        }
        throw std::invalid_argument("Unknown hit type!");
    }

    static std::string hitTypeToString(const HitType &hit) {
        switch (hit) {
        case GENERAL:
            return "Hit";
        case TWO_D_U:
            return "U View";
        case TWO_D_V:
            return "V View";
        case TWO_D_W:
            return "W View";
        }
        throw std::invalid_argument("Unknown hit class!");
    }

  protected:
    Position position;
    double time, energy;
    HitDimension dim = HitDimension::THREE_D;
    HitType type = HitType::GENERAL;
    std::string label;
    std::map<std::string, double> properties;
};
using Hits = std::vector<Hit *>;

// Convenience constructor for MC hits.
class MCHit : public Hit {
  public:
    MCHit(const Position &pos, const double pdgCode, const double t = 0, const double energy = 0)
        : Hit(pos, t, energy) {
        this->addProperties({{"PDG", pdgCode}});
    }
    MCHit(const std::array<double, 3> &pos, const double pdgCode, const double t = 0, const double energy = 0)
        : Hit(pos, t, energy) {
        this->addProperties({{"PDG", pdgCode}});
    }
};
using MCHits = std::vector<MCHit *>;

}; // namespace HepEVD

#endif // HEP_EVD_HITS_H
