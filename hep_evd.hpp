//
// hep_evd.hpp
//
// Copyright (c) 2023 Ryan Cross. All rights reserved.
// MIT License
//

#ifndef CPPHEP_EVD_H
#define CPPHEP_EVD_H

#ifndef CPPHEP_EVD_PORT
#define CPPHEP_EVD_PORT 55555
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
                BoxVolume volume(pos, params[3], params[4], params[5]);
                volumes.push_back(&volume);

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

class Hit {
  public:
    Hit(const Position &pos, double t = 0, double e = 0) : position(pos), time(t), energy(e) {}
    Hit(const std::array<double, 3> &pos, double t = 0, double e = 0) : position(pos), time(t), energy(e) {}

    void setHitType(const HitType &type) { hitType = type; }

    void setLabel(const std::string &str) { label = str; }

    void setProperties(std::map<std::string, double> props) { properties = props; }

    friend std::ostream &operator<<(std::ostream &os, Hit const &hit) {
        os << "{"
           << "\"type\": \"" << Hit::hitTypeToString(hit.hitType) << "\"," << hit.position << ","
           << "\"t\": " << hit.time << ","
           << "\"e\": " << hit.energy;

        if (!hit.label.empty())
            os << ", \"label\": " << hit.label;

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
            return "3D";
        case TRUTH:
            return "3D";
        case PRIMARY:
            return "3D";
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

class HttpEventDisplayServer {
  public:
    HttpEventDisplayServer(const DetectorGeometry &geo, const Hits &hits)
        : geometry(geo), hits(hits), mcHits({}), mcTruth("") {}
    HttpEventDisplayServer(const DetectorGeometry &geo, const Hits &hits, const MCHits &mc)
        : geometry(geo), hits(hits), mcHits(mc), mcTruth("") {}

    // Start the event display server, blocking until exit is called by the
    // server.
    void startServer();

    // Pass over the required event information.
    // TODO: Verify the information passed over.
    bool addHits(const Hits &hits) {

        if (this->hits.has_value()) {
            this->hits = hits;
            return true;
        }

        Hits newHits = this->hits.value();
        newHits.insert(newHits.end(), hits.begin(), hits.end());
        this->hits = newHits;

        return true;
    }
    bool addTruth(const MCHits &mcHits, const std::string truth = "") {
        this->mcHits = mcHits;
        this->mcTruth = truth;
        return true;
    }

  private:
    template <typename T> std::string jsonify(const std::vector<T> &data);
    template <typename T> std::string jsonify(const T &data);

    httplib::Server server;

    std::optional<DetectorGeometry> geometry;
    std::optional<Hits> hits;
    std::optional<MCHits> mcHits;
    std::optional<std::string> mcTruth;
};

template <typename T> inline std::string HttpEventDisplayServer::jsonify(const std::vector<T> &data) {

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

template <typename T> inline std::string HttpEventDisplayServer::jsonify(const T &data) {
    return HttpEventDisplayServer::jsonify(std::vector<T>({data}));
}

inline void HttpEventDisplayServer::startServer() {
    using namespace httplib;

    // Simple commands to return the currently understood server state.
    this->server.Get("/hits", [&](const Request &, Response &res) {
        res.set_content(this->jsonify<Hit>(this->hits.value()), "application/json");
    });
    this->server.Get("/mcHits", [&](const Request &, Response &res) {
        res.set_content(this->jsonify<MCHit>(this->mcHits.value()), "application/json");
    });
    this->server.Get("/geometry", [&](const Request &, Response &res) {
        res.set_content(this->jsonify<DetectorGeometry>(this->geometry.value()), "application/json");
    });

    // Management controls...
    this->server.Get("/quit", [&](const Request &, Response &res) {
        res.set_content("Goodbye!", "text/plain");
        this->server.stop();
    });

    // Finally, mount the www folder, which contains the actual HepEVD JS code.
    this->server.set_mount_point("/", "./../web");

    std::cout << "Starting a server on http://localhost:" << CPPHEP_EVD_PORT << "..." << std::endl;
    this->server.listen("localhost", CPPHEP_EVD_PORT);
    std::cout << "Server closed, continuing..." << std::endl;
}

}; // namespace HepEVD

#endif // CPPHEP_EVD_H
