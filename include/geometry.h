//
// Detector Geometry
//
// Classes to represent any form of detector geometry volume.
// A detector geometry is made up of many detector geometry volumes.

#ifndef HEP_EVD_GEOMETRY_H
#define HEP_EVD_GEOMETRY_H

#include "utils.h"

#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace HepEVD {

class GeoVolume {
  public:
    GeoVolume(const Position &pos) : position(pos) {}

    friend std::ostream &operator<<(std::ostream &os, GeoVolume const &vol) {
        os << vol.getJsonString();
        return os;
    }

    virtual Position getCenter() const = 0;
    virtual std::string getJsonString() const = 0;

  protected:
    Position position;
};

// Specialised detector geometry volume to represent any 3D box.
class BoxVolume : public GeoVolume {
  public:
    static const int ARG_COUNT = 3;

    BoxVolume(const Position &pos, double xWidth, double yWidth, double zWidth)
        : GeoVolume(pos), xWidth(xWidth), yWidth(yWidth), zWidth(zWidth) {}

    std::string getJsonString() const {
        std::stringstream os;
        os << "{"
           << "\"type\": \"box\"," << this->position << ","
           << "\"center\": {" << this->getCenter() << "},"
           << "\"xWidth\": " << this->xWidth << ","
           << "\"yWidth\": " << this->yWidth << ","
           << "\"zWidth\": " << this->zWidth << "}";

        return os.str();
    }

    Position getCenter() const { return this->position; }

  protected:
    double xWidth, yWidth, zWidth;
};

// TODO: Extend geometry model to include lines, sphere, cylinder etc.
enum VolumeType { BOX, LINE, SPHERE, CYLINDER };

using Volumes = std::vector<GeoVolume *>;
using VolumeMap = std::map<VolumeType, std::vector<double>>;

// Top-level detector geometry, with a detector being composed of
// at least one geometry volume.
class DetectorGeometry {

  public:
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
                volumes.push_back(&boxVolume);

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

    friend std::ostream &operator<<(std::ostream &os, DetectorGeometry const &geo) {
        if (geo.volumes.size() == 0)
            return os;

        for (const auto &volume : geo.volumes)
            os << *volume << ",";

        os.seekp(-1, os.cur);
        return os;
    }

  protected:
    Volumes volumes;
};

}; // namespace HepEVD

#endif // HEP_EVD_GEOMETRY_H
