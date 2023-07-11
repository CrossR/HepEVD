//
// hep_evd.hpp
//
// Copyright (c) 2023 Ryan Cross. All rights reserved.
// MIT License
//

#ifndef CPPHEP_EVD_H
#define CPPHEP_EVD_H

#ifndef CPPHEP_EVD_PORT
#define CPPHEP_EVD_PORT 5555
#endif

#define CPPHEP_EVD_VERSION "0.0.1"

#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#include "httplib.h"

// Basic Classes

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

// Abstract class to represent any form of detector geometry volume.
// A volume is just some shape at a 3D position.
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

enum HitType { THREE_D, TWO_D, TRUTH, PRIMARY };

// A hit represents an actual energy deposition in the detector.
// That is, a position but also information on the sort of hit,
// and associated energy and timing information.
class Hit {
  public:
    Hit(const Position &pos, double e = 0, double t = 0) : position(pos), time(t), energy(e) {}
    Hit(const std::array<double, 3> &pos, double e = 0, double t = 0) : position(pos), time(t), energy(e) {}

    void setHitType(const HitType &type) { hitType = type; }

    void setLabel(const std::string &str) { label = str; }

    // TODO: This may want to be extensible. I.e. string -> double + other properties (CATEGORIC, NUMERIC) etc;
    void setProperties(std::map<std::string, double> props) { properties = props; }

    friend std::ostream &operator<<(std::ostream &os, Hit const &hit) {
        os << "{"
           << "\"type\": \"" << Hit::hitTypeToString(hit.hitType) << "\"," << hit.position << ","
           << "\"time\": " << hit.time << ","
           << "\"energy\": " << hit.energy;

        if (!hit.label.empty())
            os << ", \"label\": \"" << hit.label << "\"";

        if (!hit.properties.empty()) {
            os << ", \"properties\": [";
            for (const auto &propValuePair : hit.properties)
                os << "{\"" << propValuePair.first << "\": " << propValuePair.second << "},";

            os.seekp(-1, os.cur);
            os << "]";
        }
        os << "}";

        return os;
    }

    static std::string hitTypeToString(const HitType &hit) {
        switch (hit) {
        case THREE_D:
            return "3D";
        case TWO_D:
            return "2D";
        case TRUTH:
            return "TRUTH";
        case PRIMARY:
            return "PRIMARY";
        }
        throw std::invalid_argument("Unknown hit type!");
    }

  protected:
    Position position;
    double time, energy;
    HitType hitType = HitType::THREE_D;
    std::string label;
    std::map<std::string, double> properties;
};
using Hits = std::vector<Hit>;

// Convenience constructor for MC hits.
class MCHit : public Hit {
  public:
    MCHit(const Position &pos, double t = 0, double energy = 0) : Hit(pos, t, energy) {
        this->hitType = HitType::TRUTH;
    }
    MCHit(const std::array<double, 3> &pos, double t = 0, double energy = 0) : Hit(pos, t, energy) {
        this->hitType = HitType::TRUTH;
    }
};
using MCHits = std::vector<MCHit>;

// Top level HepEVD server.
//
// Once constructed and given a detector geometry and hits,
// running the server will spin up the event display on
// localhost::CPPHEP_EVD_PORT.
class HepEVDServer {
  public:
    HepEVDServer(const DetectorGeometry &geo) : geometry(geo), hits({}), mcHits({}), mcTruth("") {}
    HepEVDServer(const DetectorGeometry &geo, const Hits &hits) : geometry(geo), hits(hits), mcHits({}), mcTruth("") {}
    HepEVDServer(const DetectorGeometry &geo, const Hits &hits, const MCHits &mc)
        : geometry(geo), hits(hits), mcHits(mc), mcTruth("") {}

    // Start the event display server, blocking until exit is called by the
    // server.
    void startServer();

    // Pass over the required event information.
    // TODO: Verify the information passed over.
    bool addHits(const Hits &inputHits) {

        if (this->hits.size() == 0) {
            this->hits = inputHits;
            return true;
        }

        Hits newHits = this->hits;
        newHits.insert(newHits.end(), inputHits.begin(), inputHits.end());
        this->hits = newHits;

        return true;
    }
    bool addTruth(const MCHits &inputMC, const std::string truth = "") {
        this->mcHits = inputMC;
        this->mcTruth = truth;
        return true;
    }

  private:
    template <typename T> std::string jsonify(const std::vector<T> &data);
    template <typename T> std::string jsonify(const T &data);

    httplib::Server server;

    DetectorGeometry geometry;
    Hits hits;
    MCHits mcHits;
    std::string mcTruth;
};

// Produce a full JSON representation of the given items.
template <typename T> inline std::string HepEVDServer::jsonify(const std::vector<T> &data) {

    if (data.size() == 0) {
        return "";
    }

    std::stringstream json_string;
    json_string << "[";

    for (const auto &dataPoint : data) {
        json_string << dataPoint << ",";
    }

    // Move the stringstream write head back one char,
    // removing the trailing comma, then close the JSON.
    json_string.seekp(-1, json_string.cur);
    json_string << "]";

    return json_string.str();
}

template <typename T> inline std::string HepEVDServer::jsonify(const T &data) {
    return HepEVDServer::jsonify(std::vector<T>({data}));
}

// Run the actual server, spinning up the API endpoints and serving the
// HTML/JS required for the event display.
inline void HepEVDServer::startServer() {
    using namespace httplib;

    // Simple commands to return the currently understood server state.
    this->server.Get("/hits", [&](const Request &, Response &res) {
        res.set_content(this->jsonify<Hit>(this->hits), "application/json");
    });
    this->server.Get("/mcHits", [&](const Request &, Response &res) {
        res.set_content(this->jsonify<MCHit>(this->mcHits), "application/json");
    });
    this->server.Get("/geometry", [&](const Request &, Response &res) {
        res.set_content(this->jsonify<DetectorGeometry>(this->geometry), "application/json");
    });

    // Management controls...
    this->server.Get("/quit", [&](const Request &, Response &res) {
        res.set_content("Goodbye!", "text/plain");
        this->server.stop();
    });

    // Finally, mount the www folder, which contains the actual HepEVD JS code.
    const std::string headerFilePath(__FILE__);
    const std::string rootFolder(headerFilePath.substr(0, headerFilePath.rfind("/")));
    std::cout << "The root folder is " << rootFolder << std::endl;
    this->server.set_mount_point("/", rootFolder + "/web/");

    std::cout << "Starting a server on http://localhost:" << CPPHEP_EVD_PORT << "..." << std::endl;
    this->server.listen("localhost", CPPHEP_EVD_PORT);
    std::cout << "Server closed, continuing..." << std::endl;
}

// High-level helper to convert Pandora objects into HEAVED ones.
#ifdef CPPHEP_EVD_PANDORA_HELPERS
namespace PandoraHelpers {

#include "Geometry/LArTPC.h"
#include "Managers/GeometryManager.h"
#include "Objects/CaloHit.h"

using HepHitMap = std::map<const pandora::CaloHit *, Hit>;

DetectorGeometry getHepEVDGeometry(const pandora::GeometryManager *manager) {

    Volumes volumes;

    for (const auto &tpcIndexPair : manager->GetLArTPCMap()) {
        const auto &lartpc = *(tpcIndexPair.second);
        BoxVolume *larTPCVolume =
            new BoxVolume(Position({lartpc.GetCenterX(), lartpc.GetCenterY(), lartpc.GetCenterZ()}), lartpc.GetWidthX(),
                          lartpc.GetWidthY(), lartpc.GetWidthZ());
        volumes.push_back(larTPCVolume);
    }

    return DetectorGeometry(volumes);
}

Hits getHepEVD2DHits(const pandora::CaloHitList *caloHits, std::string label = "", HepHitMap pandoraToCaloMap = {}) {

    Hits hits;

    for (const pandora::CaloHit *const pCaloHit : *caloHits) {
        const auto pos = pCaloHit->GetPositionVector();
        Hit hit({pos.GetX(), pos.GetY(), pos.GetZ()}, pCaloHit->GetMipEquivalentEnergy());

        if (label != "")
            hit.setLabel(label);

        hit.setHitType(HitType::TWO_D);

        hits.push_back(hit);
        pandoraToCaloMap.insert({pCaloHit, hit});
    }

    return hits;
}

}; // namespace PandoraHelpers
#endif

}; // namespace HepEVD

#endif // CPPHEP_EVD_H
