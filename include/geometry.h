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

enum VolumeType { BOX, LINE, SPHERE, CYLINDER };
NLOHMANN_JSON_SERIALIZE_ENUM(VolumeType, {{BOX, "box"}, {LINE, "line"}, {SPHERE, "sphere"}, {CYLINDER, "cylinder"}});

// Detector geometry volume to represent any 3D box.
class BoxVolume {
  public:
    static const VolumeType type = BOX;
    static const int ARG_COUNT = 3;

    BoxVolume(const Position &pos, double xWidth, double yWidth, double zWidth)
        : position(pos), xWidth(xWidth), yWidth(yWidth), zWidth(zWidth) {}
    BoxVolume(const PosArray &pos, double xWidth, double yWidth, double zWidth)
        : position(pos), xWidth(xWidth), yWidth(yWidth), zWidth(zWidth) {}

    Position getCenter() const { return this->position; }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BoxVolume, position, xWidth, yWidth, zWidth);

  private:
    Position position;
    double xWidth, yWidth, zWidth;
};

// Volumes vector to hold all possible detector geometry volumes.
using AllVolumes = std::variant<BoxVolume>;
using Volumes = std::vector<AllVolumes>;
using VolumeMap = std::map<VolumeType, std::vector<double>>;

// Define the required JSON formatters for the detector geometry volumes.
void to_json(json &j, const AllVolumes &vol) {
    std::visit([&j](const auto &vol) { j = vol; }, vol);
}
void to_json(json &j, const Volumes &vols) { j = json{{"volumes", vols}}; }

void from_json(const json &j, AllVolumes &vol) {
    Position pos = j.get<Position>();
    VolumeType type = j.at("type").get<VolumeType>();

    switch (type) {
    case BOX: {
        BoxVolume boxVolume(pos, pos.x, pos.y, pos.z);
        vol = boxVolume;
        break;
    }
    default:
        throw std::invalid_argument("Unknown volume type given!");
    }
}
void from_json(const json &j, Volumes &vols) { vols = j.at("volumes").get<Volumes>(); }

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
            case LINE:
            case SPHERE:
            case CYLINDER:
                throw std::logic_error("Geometry not yet implemented!");
            default:
                throw std::invalid_argument("Unknown volume type given!");
            }
        }
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(DetectorGeometry, volumes);

  private:
    Volumes volumes;
};

}; // namespace HepEVD

#endif // HEP_EVD_GEOMETRY_H
