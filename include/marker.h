//
// Marker
//
// Represent a "marker" on an event.
// This could be a point, say a vertex.
// A ring, to circle some point of interest.
// A line to join together multiple features in an event.

#ifndef HEP_EVD_MARKER_H
#define HEP_EVD_MARKER_H

#include "hits.h"
#include "utils.h"

#include "extern/json.hpp"
using json = nlohmann::json;

#include <array>
#include <sstream>
#include <string>
#include <variant>

namespace HepEVD {

enum MarkerType { POINT, LINE, RING };
NLOHMANN_JSON_SERIALIZE_ENUM(MarkerType, {{POINT, "Point"}, {LINE, "Line"}, {RING, "Ring"}});

// High level Marker class, defining the things that every marker has.
class Marker {
  public:
    Marker() {}
    Marker(const PosArray &pos) : position(pos) {}
    void setDim(const HitDimension &dim) { this->position.setDim(dim); }
    void setType(const HitType &type) { this->position.setType(type); }
    void setColour(const std::string colour) { this->colour = colour; }
    void setLabel(const std::string label) { this->label = label; }

  protected:
    Position position;
    std::string colour;
    std::string label;
};

// A point is just a single 2D/3D location.
class Point : public Marker {
  public:
    Point() {}
    Point(const PosArray &pos) : Marker(pos) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Point, type, position, colour, label);

  private:
    MarkerType type = POINT;
};

// A line is a 3D line between two 3D points.
class Line : public Marker {
  public:
    Line() {}
    Line(const PosArray &start, const PosArray &end) : Marker(start), end(end) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Line, type, end, position, colour, label);

  private:
    MarkerType type = LINE;

    Position end;
};

// A ring is represented by centre point, and then an inner and outer radius.
class Ring : public Marker {
  public:
    Ring() {}
    Ring(const PosArray &center, const double inner, const double outer) : Marker(center), inner(inner), outer(outer) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Ring, type, inner, outer, position, colour, label);

  private:
    MarkerType type = RING;

    double inner;
    double outer;
};

using AllMarkers = std::variant<Point, Line, Ring>;
using Markers = std::vector<AllMarkers>;

// Define the required JSON formatters for the markers variant.
static void to_json(json &j, const AllMarkers &marker) {
    std::visit([&j](const auto &marker) { j = marker; }, marker);
}
static void to_json(json &j, const Markers &markers) {

    if (markers.size() == 0) {
        j = json::array();
        return;
    }

    for (const auto &marker : markers) {
        std::visit([&j](const auto &marker) { j.push_back(marker); }, marker);
    }
}

static void from_json(const json &j, Markers &markers) {
    for (const auto &marker : j.at("markers")) {
        MarkerType type = marker.at("type").get<MarkerType>();

        switch (type) {
        case POINT: {
            Point point(marker);
            markers.push_back(point);
            break;
        }
        case LINE: {
            Line line(marker);
            markers.push_back(line);
            break;
        }
        case RING: {
            Ring ring(marker);
            markers.push_back(ring);
            break;
        }
        default:
            throw std::invalid_argument("Unknown volume type given!");
        }
    }
}


}; // namespace HepEVD

#endif // HEP_EVD_MARKER_H
