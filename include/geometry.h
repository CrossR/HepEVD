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

enum VolumeType { BOX, SPHERE, CYLINDER, TRAPEZOID };
NLOHMANN_JSON_SERIALIZE_ENUM(VolumeType,
                             {{BOX, "box"}, {SPHERE, "sphere"}, {CYLINDER, "cylinder"}, {TRAPEZOID, "trapezoid"}});

// Detector geometry volume to represent any 3D box...
class BoxVolume {
  public:
    static const VolumeType volumeType = BOX;
    static const int ARG_COUNT = 3;

    BoxVolume() {}
    BoxVolume(const Position &pos, double xWidth, double yWidth, double zWidth)
        : position(pos), xWidth(xWidth), yWidth(yWidth), zWidth(zWidth) {}
    BoxVolume(const PosArray &pos, double xWidth, double yWidth, double zWidth)
        : position(pos), xWidth(xWidth), yWidth(yWidth), zWidth(zWidth) {}

    Position getCenter() const { return this->position; }
    double getXWidth() const { return this->xWidth; }
    double getYWidth() const { return this->yWidth; }
    double getZWidth() const { return this->zWidth; }

    // Use custom to/from_json to allow including the volume type.
    friend void to_json(json &j, const BoxVolume &box) {
        j["volumeType"] = BOX;
        j["position"] = box.position;
        j["xWidth"] = box.xWidth;
        j["yWidth"] = box.yWidth;
        j["zWidth"] = box.zWidth;
    }

    friend void from_json(const json &j, BoxVolume &box) {
        j.at("position").get_to(box.position);
        j.at("xWidth").get_to(box.xWidth);
        j.at("yWidth").get_to(box.yWidth);
        j.at("zWidth").get_to(box.zWidth);
    }

  private:
    Position position;
    double xWidth, yWidth, zWidth;
};

// And a 3D cylinder...
class CylinderVolume {
  public:
    static const VolumeType volumeType = CYLINDER;
    static const int ARG_COUNT = 3;

    CylinderVolume() {}
    CylinderVolume(const Position &pos, double radius, double height) : position(pos), radius(radius), height(height) {}
    CylinderVolume(const PosArray &pos, double radius, double height) : position(pos), radius(radius), height(height) {}

    Position getCenter() const { return this->position; }
    double getRadius() const { return this->radius; }
    double getHeight() const { return this->height; }

    // Use custom to/from_json to allow including the volume type.
    friend void to_json(json &j, const CylinderVolume &cylinder) {
        j["volumeType"] = CYLINDER;
        j["position"] = cylinder.position;
        j["radius"] = cylinder.radius;
        j["height"] = cylinder.height;
    }

    friend void from_json(const json &j, CylinderVolume &cylinder) {
        j.at("position").get_to(cylinder.position);
        j.at("radius").get_to(cylinder.radius);
        j.at("height").get_to(cylinder.height);
    }

  private:
    Position position;
    double radius, height;
};

// Trapezoid volume...
class TrapezoidVolume {
  public:
    static const VolumeType volumeType = TRAPEZOID;
    static const int ARG_COUNT = 5;

    TrapezoidVolume() {}
    TrapezoidVolume(const Position &pos, const Position &topLeft, const Position &topRight, const Position &bottomLeft,
                    const Position &bottomRight)
        : position(pos), topLeft(topLeft), topRight(topRight), bottomLeft(bottomLeft), bottomRight(bottomRight) {}
    TrapezoidVolume(const PosArray &pos, const PosArray &topLeft, const PosArray &topRight, const PosArray &bottomLeft,
                    const PosArray &bottomRight)
        : position(pos), topLeft(topLeft), topRight(topRight), bottomLeft(bottomLeft), bottomRight(bottomRight) {}
    TrapezoidVolume(const Position &pos, const std::vector<Position> &vertices)
        : position(pos), topLeft(vertices[0]), topRight(vertices[1]), bottomLeft(vertices[2]),
          bottomRight(vertices[3]) {}

    Position getCenter() const { return this->position; }
    Position getTopLeft() const { return this->topLeft; }
    Position getTopRight() const { return this->topRight; }
    Position getBottomLeft() const { return this->bottomLeft; }
    Position getBottomRight() const { return this->bottomRight; }

    // Use custom to/from_json to allow including the volume type.
    friend void to_json(json &j, const TrapezoidVolume &trapezoid) {
        j["volumeType"] = TRAPEZOID;
        j["position"] = trapezoid.position;
        j["topLeft"] = trapezoid.topLeft;
        j["topRight"] = trapezoid.topRight;
        j["bottomLeft"] = trapezoid.bottomLeft;
        j["bottomRight"] = trapezoid.bottomRight;
    }

    friend void from_json(const json &j, TrapezoidVolume &trapezoid) {
        j.at("position").get_to(trapezoid.position);
        j.at("topLeft").get_to(trapezoid.topLeft);
        j.at("topRight").get_to(trapezoid.topRight);
        j.at("bottomLeft").get_to(trapezoid.bottomLeft);
        j.at("bottomRight").get_to(trapezoid.bottomRight);
    }

  private:
    Position position;
    Position topLeft, topRight, bottomLeft, bottomRight;
};

// Volumes vector to hold all possible detector geometry volumes.
using AllVolumes = std::variant<BoxVolume, CylinderVolume, TrapezoidVolume>;
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
    DetectorGeometry() : volumes({}) {}
    DetectorGeometry(Volumes &vols) : volumes(vols) {}

    ~DetectorGeometry() { this->volumes.clear(); }

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
                volumes.push_back(boxVolume);

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

    int size() { return this->volumes.size(); }
    void clear() { return this->volumes.clear(); }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(DetectorGeometry, volumes);

  private:
    Volumes volumes;
};

}; // namespace HepEVD

#endif // HEP_EVD_GEOMETRY_H
