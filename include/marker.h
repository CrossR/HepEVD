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
    Marker(const Position &pos) : position(pos) {}
    virtual void setDim(const HitDimension &dim) { this->position.setDim(dim); }
    virtual void setHitType(const HitType &hitType) { this->position.setHitType(hitType); }
    void setColour(const std::string colour) { this->colour = colour; }
    void setLabel(const std::string label) { this->label = label; }

  protected:
    Position position;
    std::string colour;
    std::string label;
};

// A point is just a single 2D/3D location.
class Point : public Marker {
    friend class Line;
    friend class Ring;

  public:
    Point() {}
    Point(const PosArray &pos) : Marker(pos) {}
    Point(const PosArray &pos, const HitDimension &hitDim, const HitType &hitType) : Marker(pos) {
        this->position.setDim(hitDim);
        this->position.setHitType(hitType);
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Point, markerType, position, colour, label);

  private:
    MarkerType markerType = POINT;
};

// A line is a 3D line between two 3D points.
class Line : public Marker {
  public:
    Line() {}
    Line(const PosArray &start, const PosArray &end) : Marker(start), end(end) {}
    Line(const Point &start, const Point &end) : Marker(start.position), end(end.position) {}

    void setDim(const HitDimension &dim) override {
        this->position.setDim(dim);
        this->end.setDim(dim);
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Line, markerType, end, position, colour, label);

  private:
    MarkerType markerType = LINE;

    Position end;
};

// A ring is represented by centre point, and then an inner and outer radius.
class Ring : public Marker {
  public:
    Ring() {}
    Ring(const PosArray &center, const double inner, const double outer) : Marker(center), inner(inner), outer(outer) {}

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Ring, markerType, inner, outer, position, colour, label);

  private:
    MarkerType markerType = RING;

    double inner;
    double outer;
};

using AllMarkers = std::variant<Point, Line, Ring>;
using Markers = std::vector<AllMarkers>;

// Define the required JSON formatters for the markers variant.
inline static void to_json(json &j, const AllMarkers &marker) {
    std::visit([&j](const auto &marker) { j = marker; }, marker);
}
inline static void to_json(json &j, const Markers &markers) {

    if (markers.size() == 0) {
        j = json::array();
        return;
    }

    for (const auto &marker : markers) {
        std::visit([&j](const auto &marker) { j.push_back(marker); }, marker);
    }
}

inline static void from_json(const json &j, Markers &markers) {
    if (!j.is_array())
        throw std::invalid_argument("Markers must be an array!");

    for (const auto &marker : j) {
        MarkerType markerType = marker.at("markerType").get<MarkerType>();

        switch (markerType) {
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
            throw std::invalid_argument("Unknown marker type given!");
        }
    }
}

}; // namespace HepEVD

#endif // HEP_EVD_MARKER_H
