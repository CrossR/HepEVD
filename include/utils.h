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

enum class PropertyType { CATEGORIC, NUMERIC };
NLOHMANN_JSON_SERIALIZE_ENUM(PropertyType,
                             {{PropertyType::CATEGORIC, "CATEGORIC"}, {PropertyType::NUMERIC, "NUMERIC"}});

// Store a 3D position, and include a helper for JSON production.
class Position {

  public:
    Position() : x(0.0), y(0.0), z(0.0) {}
    Position(const PosArray &pos) : x(pos[0]), y(pos[1]), z(pos[2]) {}

    void setDim(HitDimension d) { dim = d; }
    void setHitType(HitType t) { hitType = t; }

    // Implement comparison operators so we can sort Positions.
    bool operator<(const Position &other) const {
        if (this->x != other.x)
            return this->x < other.x;

        if (this->y != other.y)
            return this->y < other.y;

        return this->z < other.z;
    }

    // Implement equality operators so we can remove duplicates.
    bool operator==(const Position &other) const {
        return this->x == other.x && this->y == other.y && this->z == other.z;
    }

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

// General helper definition.
using Positions = std::vector<Position>;

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

static std::string getCWD() {
    char buff[FILENAME_MAX];

    if (getcwd(buff, FILENAME_MAX) != 0) {
        return std::string(buff);
    }

    return "";
}

// A basic map of PDG codes to particle names in LaTeX, and if they are visible
// in the detector.  This is used to provide a human-readable name for
// particles.
static const std::unordered_map<int, std::tuple<bool, std::string>> hepEVD_pdgMap = {
    {11, {true, "e^-"}},     {12, {false, "\\nu_e"}},     {-11, {true, "e^+"}},     {-12, {false, "\\bar{\\nu}_e"}},
    {13, {true, "\\mu^-"}},  {14, {false, "\\nu_\\mu"}},  {-13, {true, "\\mu^+"}},  {-14, {false, "\\bar{\\nu}_\\mu"}},
    {15, {true, "\\tau^-"}}, {16, {false, "\\nu_\\tau"}}, {-15, {true, "\\tau^+"}}, {-16, {false, "\\bar{\\nu}_\\tau"}},
    {22, {true, "\\gamma"}}, {111, {true, "\\pi^0"}},     {211, {true, "\\pi^+"}},  {-211, {true, "\\pi^-"}},
    {2212, {true, "p"}},     {2112, {false, "n"}},
};

// Return true if the PDG should be included in any output text.
// This is a bit opinionated because:
//  - Includes neutrinos by default.
//  - Includes Pi0s, as its cleaner than including their decay products.
static bool pdgIsVisible(const int pdgCode, const bool includeNeutrino = true) {
    auto it = hepEVD_pdgMap.find(pdgCode);

    if (it == hepEVD_pdgMap.end() && pdgCode < 1000000000) {
        std::cout << "HepEVD: Unknown PDG code: " << pdgCode << std::endl;
        return true;
    } else if (pdgCode >= 1000000000) {
        return false;
    }

    if (std::abs(pdgCode) == 12 || std::abs(pdgCode) == 14 || std::abs(pdgCode) == 16)
        return includeNeutrino;

    return std::get<0>(it->second);
}

static std::string pdgToString(const int pdgCode, const double energy = -1.0, const std::string units = "GeV") {

    std::string res;
    auto it = hepEVD_pdgMap.find(pdgCode);

    if (it == hepEVD_pdgMap.end())
        res = "Unknown PDG: " + std::to_string(pdgCode);
    else
        res = std::get<1>(it->second);

    if (energy > 0.0)
        res += " (\\text{" + std::to_string(energy) + " " + units + "})";

    return res;
}

}; // namespace HepEVD

#endif // HEP_EVD_POSITION_H
