//
// Utilities
//
// General utils, used throughout the event display.

#ifndef HEP_EVD_POSITION_H
#define HEP_EVD_POSITION_H

#include "extern/json.hpp"
using json = nlohmann::json;

#include <array>
#include <ostream>

namespace HepEVD {

using PosArray = std::array<double, 3>;

enum HitDimension { THREE_D, TWO_D };
NLOHMANN_JSON_SERIALIZE_ENUM(HitDimension, {{THREE_D, "3D"}, {TWO_D, "2D"}});

enum HitType { GENERAL, TWO_D_U, TWO_D_V, TWO_D_W };
NLOHMANN_JSON_SERIALIZE_ENUM(HitType,
                             {{GENERAL, "Hit"}, {TWO_D_U, "U View"}, {TWO_D_V, "V View"}, {TWO_D_W, "W View"}});

// Store a 3D position, and include a helper for JSON production.
class Position {

  public:
    Position() : x(0.0), y(0.0), z(0.0) {}
    Position(const PosArray &pos) : x(pos[0]), y(pos[1]), z(pos[2]) {}

    friend std::ostream &operator<<(std::ostream &os, Position const &pos) {
        os << "\"x\": " << pos.x << ","
           << "\"y\": " << pos.y << ","
           << "\"z\": " << pos.z;
        return os;
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Position, x, y, z);

    double x, y, z;
};

}; // namespace HepEVD

#endif // HEP_EVD_POSITION_H
