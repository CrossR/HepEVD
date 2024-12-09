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
    Marker(const PosArray &pos) : m_position(pos) {}
    Marker(const Position &pos) : m_position(pos) {}
    virtual void setDim(const HitDimension &dim) { this->m_position.setDim(dim); }
    virtual void setHitType(const HitType &hitType) { this->m_position.setHitType(hitType); }
    void setColour(const std::string colour) { this->m_colour = colour; }
    void setLabel(const std::string label) { this->m_label = label; }
    std::string getColour() const { return this->m_colour; }
    std::string getLabel() const { return this->m_label; }

  protected:
    Position m_position;
    std::string m_colour;
    std::string m_label;
};

// A point is just a single 2D/3D location.
class Point : public Marker {
    friend class Line;
    friend class Ring;

  public:
    Point() {}
    Point(const PosArray &pos) : Marker(pos) {}
    Point(const PosArray &pos, const HitDimension &hitDim, const HitType &hitType) : Marker(pos) {
        this->m_position.setDim(hitDim);
        this->m_position.setHitType(hitType);
    }

    // to_json and from_json for Point.
    friend void to_json(json &j, const Point &point) {
        j["markerType"] = POINT;
        j["position"] = point.m_position;
        j["colour"] = point.m_colour;
        j["label"] = point.m_label;
    }

    friend void from_json(const json &j, Point &point) {
        j.at("markerType").get_to(point.m_markerType);
        j.at("position").get_to(point.m_position);
        j.at("colour").get_to(point.m_colour);
        j.at("label").get_to(point.m_label);
    }

  private:
    MarkerType m_markerType = POINT;
};

// A line is a 3D line between two 3D points.
class Line : public Marker {
  public:
    Line() {}
    Line(const PosArray &start, const PosArray &end) : Marker(start), m_end(end) {}
    Line(const Point &start, const Point &end) : Marker(start.m_position), m_end(end.m_position) {}

    void setDim(const HitDimension &dim) override {
        this->m_position.setDim(dim);
        this->m_end.setDim(dim);
    }

    // to_json and from_json for Line.
    friend void to_json(json &j, const Line &line) {
        j["markerType"] = LINE;
        j["position"] = line.m_position;
        j["end"] = line.m_end;
        j["colour"] = line.m_colour;
        j["label"] = line.m_label;
    }

    friend void from_json(const json &j, Line &line) {
        j.at("markerType").get_to(line.m_markerType);
        j.at("position").get_to(line.m_position);
        j.at("end").get_to(line.m_end);
        j.at("colour").get_to(line.m_colour);
        j.at("label").get_to(line.m_label);
    }

  private:
    MarkerType m_markerType = LINE;

    Position m_end;
};

// A ring is represented by centre point, and then an inner and outer radius.
class Ring : public Marker {
  public:
    Ring() {}
    Ring(const PosArray &center, const double inner, const double outer)
        : Marker(center), m_inner(inner), m_outer(outer) {}

    // to_json and from_json for Ring.
    friend void to_json(json &j, const Ring &ring) {
        j["markerType"] = RING;
        j["position"] = ring.m_position;
        j["inner"] = ring.m_inner;
        j["outer"] = ring.m_outer;
        j["colour"] = ring.m_colour;
        j["label"] = ring.m_label;
    }

    friend void from_json(const json &j, Ring &ring) {
        j.at("markerType").get_to(ring.m_markerType);
        j.at("position").get_to(ring.m_position);
        j.at("inner").get_to(ring.m_inner);
        j.at("outer").get_to(ring.m_outer);
        j.at("colour").get_to(ring.m_colour);
        j.at("label").get_to(ring.m_label);
    }

  private:
    MarkerType m_markerType = RING;

    double m_inner;
    double m_outer;
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
