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

enum VolumeType { BOX, SPHERE, CYLINDER };
NLOHMANN_JSON_SERIALIZE_ENUM(VolumeType, {{BOX, "box"}, {SPHERE, "sphere"}, {CYLINDER, "cylinder"}});

// Detector geometry volume to represent any 3D box.
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

// Volumes vector to hold all possible detector geometry volumes.
using AllVolumes = std::variant<BoxVolume>;
using Volumes = std::vector<AllVolumes>;
using VolumeMap = std::map<VolumeType, std::vector<double>>;

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
    if (! j.is_array())
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

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(DetectorGeometry, volumes);

  private:
    Volumes volumes;
};

}; // namespace HepEVD

#endif // HEP_EVD_GEOMETRY_H
