//
// Utilities
//
// General utils, used throughout the event display.

#ifndef HEP_EVD_POSITION_H
#define HEP_EVD_POSITION_H

#include "config.h"

#include "extern/httplib.h"
#include "extern/json.hpp"
using json = nlohmann::json;

#include <array>
#include <ostream>
#include <random>

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
    void setHitType(HitType t) { hitType = t; }

    // When converting to JSON, we want to convert 2D positoins to use
    // XY, not XZ.
    friend void to_json(json &j, const Position &pos) {
        if (pos.dim == THREE_D) {
            j = {{"x", pos.x}, {"y", pos.y}, {"z", pos.z}, {"dim", pos.dim}, {"hitType", pos.hitType}};
            return;
        }

        j = {{"x", pos.x}, {"y", pos.z}, {"z", 0.0}, {"dim", pos.dim}, {"hitType", pos.hitType}};
        return;
    }

    // That means we need to convert from XY to XZ when reading from JSON.
    friend void from_json(const json &j, Position &pos) {
        j.at("dim").get_to(pos.dim);
        j.at("hitType").get_to(pos.hitType);

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
    HitType hitType = GENERAL;
};

// General templated utility function to POST data to a URL.
template <typename T> bool postData(const std::string &endPoint, const T &data) {
    const std::string server = "localhost:" + std::to_string(EVD_PORT());
    httplib::Client cli(server);
    auto res = cli.Post(endPoint, json(data).dump(), "application/json");
    return res.error() == httplib::Error::Success;
}

// Very basic UUID generator.
static std::string getUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    const char *v = "0123456789abcdef";
    const bool dash[] = {0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0};

    std::string res;
    for (int i = 0; i < 16; i++) {
        if (dash[i])
            res += "-";
        res += v[dis(gen)];
        res += v[dis(gen)];
    }

    return res;
}

}; // namespace HepEVD

#endif // HEP_EVD_POSITION_H
