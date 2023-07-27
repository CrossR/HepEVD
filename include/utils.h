//
// Utilities
//
// General utils, used throughout the event display.

#ifndef HEP_EVD_POSITION_H
#define HEP_EVD_POSITION_H

namespace HepEVD {

// Store a 3D position, and include a helper for JSON production.
class Position {

  public:
    Position(const std::array<double, 3> &pos) : x(pos[0]), y(pos[1]), z(pos[2]) {}

    friend std::ostream &operator<<(std::ostream &os, Position const &pos) {
        os << "\"x\": " << pos.x << ","
           << "\"y\": " << pos.y << ","
           << "\"z\": " << pos.z;
        return os;
    }

    double x, y, z;
};

}; // namespace HepEVD

#endif // HEP_EVD_POSITION_H
