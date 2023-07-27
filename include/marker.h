//
// Marker
//
// Represent a "marker" on an event.
// This could be a point, say a vertex.
// A ring, to circle some point of interest.
// A line to join together multiple features in an event.

#ifndef HEP_EVD_MARKER_H
#define HEP_EVD_MARKER_H

#include "utils.h"

#include <sstream>
#include <string>

namespace HepEVD {

// High level Marker class, defining the things that every marker has.
class Marker {
  public:
    void setColour(const std::string colour) { this->colour = colour; }
    void setLabel(const std::string label) { this->label = label; }

    friend std::ostream &operator<<(std::ostream &os, Marker const &marker) {
        os << "{"
           << "\"label\": " << marker.label << ","
           << "\"colour\": " << marker.colour << "," << marker.getJsonString() << "}";
        return os;
    }

    virtual std::string getJsonString() const = 0;

  protected:
    std::string colour;
    std::string label;
};

using Markers = std::vector<Marker *>;

// A point is just a single 3D location.
class Point {
  public:
    Point(const Position &pos) : position(pos) {}

    std::string getJsonString() const {
        std::stringstream os;
        os << "\"type\": \"point\"," << this->position;
        return os.str();
    }

  private:
    Position position;
};

// A line is a 3D line between two 3D points.
class Line {
  public:
    Line(const Position &start, const Position &end) : start(start), end(end) {}

    std::string getJsonString() const {
        std::stringstream os;
        os << "\"type\": \"line\","
           << "\"start\": {" << this->start << "}, "
           << "\"end\": {" << this->end << "}";
        return os.str();
    }

  private:
    Position start;
    Position end;
};

// A ring is represented by centre point, and then an inner and outer radius.
class Ring {
  public:
    Ring(const Position &center, const float inner, const float outer) : center(center), inner(inner), outer(outer) {}

    std::string getJsonString() const {
        std::stringstream os;
        os << "\"type\": \"ring\"," << this->center << ", "
           << "\"inner\": " << this->inner << ","
           << "\"outer\": " << this->outer;
        return os.str();
    }

  private:
    Position center;
    float inner;
    float outer;
};

}; // namespace HepEVD

#endif // HEP_EVD_MARKER_H
