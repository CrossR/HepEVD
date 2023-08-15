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

    void setDim(HitDimension d) { dim = d; }
    void setType(HitType t) { type = t; }

    // When converting to JSON, we want to convert 2D positoins to use
    // XY, not XZ.
    friend void to_json(json &j, const Position &pos) {
        if (pos.dim == THREE_D) {
            j = {{"x", pos.x}, {"y", pos.y}, {"z", pos.z}, {"dim", pos.dim}, {"type", pos.type}};
            return;
        }

        j = {{"x", pos.x}, {"y", pos.z}, {"z", 0.0}, {"dim", pos.dim}, {"type", pos.type}};
        return;
    }

    // That means we need to convert from XY to XZ when reading from JSON.
    friend void from_json(const json &j, Position &pos) {
        j.at("dim").get_to(pos.dim);
        j.at("type").get_to(pos.type);

        if (pos.dim == THREE_D) {
            j.at("x").get_to(pos.x);
            j.at("y").get_to(pos.y);
            j.at("z").get_to(pos.z);
        } else {
            j.at("x").get_to(pos.x);
            j.at("y").get_to(pos.z);
            pos.y = 0.0;
        }

        return;
    }

    double x, y, z;
    HitDimension dim = THREE_D;
    HitType type = GENERAL;
};

}; // namespace HepEVD

#endif // HEP_EVD_POSITION_H
