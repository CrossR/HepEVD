//
// Detector Geometry
//
// Classes to represent any form of detector geometry volume.
// A detector geometry is made up of many detector geometry volumes.

#ifndef HEP_EVD_GEOMETRY_H
#define HEP_EVD_GEOMETRY_H

#include "utils.h"

#include "extern/json.hpp"
using json = nlohmann::json;

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace HepEVD {

enum VolumeType { BOX, SPHERE, CYLINDER, TRAPEZOID, RECTANGLE2D };
NLOHMANN_JSON_SERIALIZE_ENUM(
    VolumeType,
    {{BOX, "box"}, {SPHERE, "sphere"}, {CYLINDER, "cylinder"}, {TRAPEZOID, "trapezoid"}, {RECTANGLE2D, "rectangle2D"}})

// Detector geometry volume to represent any 3D box...
class BoxVolume {
  public:
    static const VolumeType volumeType = BOX;
    static const int ARG_COUNT = 3;

    BoxVolume() {}
    BoxVolume(const Position &pos, double xWidth, double yWidth, double zWidth)
        : m_position(pos), m_xWidth(xWidth), m_yWidth(yWidth), m_zWidth(zWidth) {}
    BoxVolume(const PosArray &pos, double xWidth, double yWidth, double zWidth)
        : m_position(pos), m_xWidth(xWidth), m_yWidth(yWidth), m_zWidth(zWidth) {}

    Position getCenter() const { return this->m_position; }
    double getXWidth() const { return this->m_xWidth; }
    double getYWidth() const { return this->m_yWidth; }
    double getZWidth() const { return this->m_zWidth; }

    // Use custom to/from_json to allow including the volume type.
    friend void to_json(json &j, const BoxVolume &box) {
        j["volumeType"] = BOX;
        j["position"] = box.m_position;
        j["xWidth"] = box.m_xWidth;
        j["yWidth"] = box.m_yWidth;
        j["zWidth"] = box.m_zWidth;
    }

    friend void from_json(const json &j, BoxVolume &box) {
        j.at("position").get_to(box.m_position);
        j.at("xWidth").get_to(box.m_xWidth);
        j.at("yWidth").get_to(box.m_yWidth);
        j.at("zWidth").get_to(box.m_zWidth);
    }

  private:
    Position m_position;
    double m_xWidth, m_yWidth, m_zWidth;
};

// And a 3D cylinder...
class CylinderVolume {
  public:
    static const VolumeType volumeType = CYLINDER;
    static const int ARG_COUNT = 3;

    CylinderVolume() {}
    CylinderVolume(const Position &pos, double radius, double height) : m_position(pos), m_radius(radius), m_height(height) {}
    CylinderVolume(const PosArray &pos, double radius, double height) : m_position(pos), m_radius(radius), m_height(height) {}

    Position getCenter() const { return this->m_position; }
    double getRadius() const { return this->m_radius; }
    double getHeight() const { return this->m_height; }

    // Use custom to/from_json to allow including the volume type.
    friend void to_json(json &j, const CylinderVolume &cylinder) {
        j["volumeType"] = CYLINDER;
        j["position"] = cylinder.m_position;
        j["radius"] = cylinder.m_radius;
        j["height"] = cylinder.m_height;
    }

    friend void from_json(const json &j, CylinderVolume &cylinder) {
        j.at("position").get_to(cylinder.m_position);
        j.at("radius").get_to(cylinder.m_radius);
        j.at("height").get_to(cylinder.m_height);
    }

  private:
    Position m_position;
    double m_radius, m_height;
};

// Trapezoid volume...
class TrapezoidVolume {
  public:
    static const VolumeType volumeType = TRAPEZOID;
    static const int ARG_COUNT = 5;

    TrapezoidVolume() {}
    TrapezoidVolume(const Position &pos, const Position &topLeft, const Position &topRight, const Position &bottomLeft,
                    const Position &bottomRight)
        : m_position(pos), m_topLeft(topLeft), m_topRight(topRight), m_bottomLeft(bottomLeft), m_bottomRight(bottomRight) {}
    TrapezoidVolume(const PosArray &pos, const PosArray &topLeft, const PosArray &topRight, const PosArray &bottomLeft,
                    const PosArray &bottomRight)
        : m_position(pos), m_topLeft(topLeft), m_topRight(topRight), m_bottomLeft(bottomLeft), m_bottomRight(bottomRight) {}
    TrapezoidVolume(const Position &pos, const std::vector<Position> &vertices)
        : m_position(pos), m_topLeft(vertices[0]), m_topRight(vertices[1]), m_bottomLeft(vertices[2]),
          m_bottomRight(vertices[3]) {}

    Position getCenter() const { return this->m_position; }
    Position getTopLeft() const { return this->m_topLeft; }
    Position getTopRight() const { return this->m_topRight; }
    Position getBottomLeft() const { return this->m_bottomLeft; }
    Position getBottomRight() const { return this->m_bottomRight; }

    // Use custom to/from_json to allow including the volume type.
    friend void to_json(json &j, const TrapezoidVolume &trapezoid) {
        j["volumeType"] = TRAPEZOID;
        j["position"] = trapezoid.m_position;
        j["topLeft"] = trapezoid.m_topLeft;
        j["topRight"] = trapezoid.m_topRight;
        j["bottomLeft"] = trapezoid.m_bottomLeft;
        j["bottomRight"] = trapezoid.m_bottomRight;
    }

    friend void from_json(const json &j, TrapezoidVolume &trapezoid) {
        j.at("position").get_to(trapezoid.m_position);
        j.at("topLeft").get_to(trapezoid.m_topLeft);
        j.at("topRight").get_to(trapezoid.m_topRight);
        j.at("bottomLeft").get_to(trapezoid.m_bottomLeft);
        j.at("bottomRight").get_to(trapezoid.m_bottomRight);
    }

  protected:
    Position m_position;
    Position m_topLeft, m_topRight, m_bottomLeft, m_bottomRight;
};

// Rectangle2D volume...
// These are split out from the Trapezoid + Box volumes as they are
// treated differently (mostly because they are 2D, but rendered in 3D).
// Based on the TrapezoidVolume class, inheriting from it.
class Rectangle2DVolume : public TrapezoidVolume {
    using TrapezoidVolume::TrapezoidVolume;

  public:
    static const VolumeType volumeType = RECTANGLE2D;

    // Use custom to/from_json to allow including the volume type.
    friend void to_json(json &j, const Rectangle2DVolume &rect) {
        j["volumeType"] = RECTANGLE2D;
        j["position"] = rect.getCenter();
        j["topLeft"] = rect.getTopLeft();
        j["topRight"] = rect.getTopRight();
        j["bottomLeft"] = rect.getBottomLeft();
        j["bottomRight"] = rect.getBottomRight();
    }

    friend void from_json(const json &j, Rectangle2DVolume &rect) {
        j.at("position").get_to(rect.m_position);
        j.at("topLeft").get_to(rect.m_topLeft);
        j.at("topRight").get_to(rect.m_topRight);
        j.at("bottomLeft").get_to(rect.m_bottomLeft);
        j.at("bottomRight").get_to(rect.m_bottomRight);
    }
};

// Volumes vector to hold all possible detector geometry volumes.
using AllVolumes = std::variant<BoxVolume, CylinderVolume, TrapezoidVolume, Rectangle2DVolume>;
using Volumes = std::vector<AllVolumes>;
using VolumeMap = std::vector<std::pair<VolumeType, std::vector<double>>>;

// Define the required JSON formatters for the detector geometry volumes.
inline static void to_json(json &j, const AllVolumes &vol) {
    std::visit([&j](const auto &vol) { j = vol; }, vol);
}
inline static void to_json(json &j, const Volumes &vols) {

    if (vols.size() == 0) {
        j = json::array();
        return;
    }
    for (const auto &vol : vols) {
        std::visit([&j](const auto &vol) { j.push_back(vol); }, vol);
    }
}

// Parse the vector of Detector volumes...
inline static void from_json(const json &j, Volumes &vols) {
    if (!j.is_array())
        throw std::invalid_argument("Volumes must be an array!");

    for (const auto &vol : j) {
        VolumeType type = vol.at("volumeType").get<VolumeType>();

        switch (type) {
        case BOX: {
            BoxVolume boxVolume(vol);
            vols.push_back(boxVolume);
            break;
        }
        default:
            throw std::invalid_argument("Unknown volume type given!");
        }
    }
}

// Top-level detector geometry, with a detector being composed of
// at least one geometry volume.
class DetectorGeometry {

  public:
    DetectorGeometry() : m_volumes({}) {}
    DetectorGeometry(Volumes &vols) : m_volumes(vols) {}

    ~DetectorGeometry() { this->m_volumes.clear(); }

    DetectorGeometry(VolumeMap &volumeMap) {
        for (const auto &volume : volumeMap) {

            std::vector<double> params = volume.second;
            VolumeType volumeType(volume.first);

            if (params.size() < 3)
                throw std::invalid_argument("All volumes need at least a position!");

            Position pos({params[0], params[1], params[2]});

            switch (volumeType) {
            case BOX: {
                if (volume.second.size() - 3 != BoxVolume::ARG_COUNT)
                    throw std::invalid_argument("A box volume needs 6 inputs!");
                BoxVolume boxVolume(pos, params[3], params[4], params[5]);
                m_volumes.push_back(boxVolume);

                break;
            }
            case SPHERE:
            case CYLINDER:
                throw std::logic_error("Geometry not yet implemented!");
            default:
                throw std::invalid_argument("Unknown volume type given!");
            }
        }
    }

    int size() { return this->m_volumes.size(); }
    void clear() { return this->m_volumes.clear(); }

    // Define to/from_json.
    friend void to_json(json &j, const DetectorGeometry &geom) { j["volumes"] = geom.m_volumes; }
    friend void from_json(const json &j, DetectorGeometry &geom) { j.at("volumes").get_to(geom.m_volumes); }

  private:
    Volumes m_volumes;
};

}; // namespace HepEVD

#endif // HEP_EVD_GEOMETRY_H
