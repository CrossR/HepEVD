//
// hep_evd.hpp
//
// Copyright (c) 2023 Ryan Cross. All rights reserved.
// MIT License
//

#ifndef CPPHEP_EVD_H
#define CPPHEP_EVD_H

#include <sstream>
#ifndef CPPHEP_EVD_PORT
#define CPPHEP_EVD_PORT 55555
#endif

#define CPPHEP_EVD_VERSION "0.0.1"

#include <iostream>
#include <optional>
#include <string>

#include "httplib.h"

// Basic Classes

namespace HepEVD {

using Position = std::array<float, 3>;

class GeoVolume {
  public:
    GeoVolume(const Position &pos) : position(pos) {}

    friend std::ostream &operator<<(std::ostream &os, GeoVolume const &vol) {
        os << vol.getJsonString();
        return os;
    }

  protected:
    std::string getJsonStringStart() const {
        std::stringstream os;
        os << "{"
           << "\"x\": " << this->position[0] << ","
           << "\"y\": " << this->position[1] << ","
           << "\"z\": " << this->position[2] << ",";

        return os.str();
    }

    virtual std::string getJsonString() const = 0;

    Position position;
};

using Volumes = std::vector<std::shared_ptr<GeoVolume>>;

class BoxVolume : GeoVolume {
  public:
    BoxVolume(const Position &pos, float xWidth, float yWidth, float zWidth)
        : GeoVolume(pos), xWidth(xWidth), yWidth(yWidth), zWidth(zWidth) {}

    std::string getJsonString() {
        std::stringstream os;
        os << this->getJsonStringStart() << "\"xWidth\": " << this->xWidth << ","
           << "\"yWidth\": " << this->yWidth << ","
           << "\"zWidth\": " << this->zWidth << "}";

        return os.str();
    }

  protected:
    float xWidth, yWidth, zWidth;
};

// TODO: Extend geometry model to include lines, sphere, cylinder etc.

class DetectorGeometry {

    DetectorGeometry(Volumes &vols) : volumes(vols) {}

    friend std::ostream &operator<<(std::ostream &os, DetectorGeometry const &geo) {
        if (!geo.volumes.has_value())
            return os;

        for (const auto &volume : geo.volumes.value())
            os << *volume;

        return os;
    }

  protected:
    std::optional<Volumes> volumes;
};

enum HitType { GENERAL, TRUTH, PRIMARY, U_VIEW, V_VIEW, W_VIEW };

class Hit {
  public:
    Hit(const Position &pos, float t = 0, float e = 0) : position(pos), time(t), energy(e) {}

    void setHitType(const HitType &type) { hitType = type; }

    void setLabel(const std::string &str) { label = str; }

    void setProperties(std::map<std::string, float> props) { properties = props; }

    friend std::ostream &operator<<(std::ostream &os, Hit const &hit) {
        os << "{";
        os << "\"x\": " << hit.position[0] << ",";
        os << "\"y\": " << hit.position[1] << ",";
        os << "\"z\": " << hit.position[2] << ",";
        os << "\"t\": " << hit.time << ",";
        os << "\"e\": " << hit.energy;

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
        ;
        return os;
    }

  protected:
    Position position;
    float time, energy;
    HitType hitType = HitType::GENERAL;
    std::string label;
    std::map<std::string, float> properties;
};
using Hits = std::vector<Hit>;

class MCHit : public Hit {
  public:
    MCHit(const Position &pos, float t = 0, float energy = 0) : Hit(pos, t, energy) { this->hitType = HitType::TRUTH; }
};
using MCHits = std::vector<MCHit>;

class HttpEventDisplayServer {
  public:
    HttpEventDisplayServer() {}

    // Start the event display server, blocking until exit is called by the
    // server.
    void startServer();

    // Pass over the required event information.
    // TODO: Verify the information passed over.
    bool assignGeometry(const DetectorGeometry &geo) {
        this->geometry = geo;
        return true;
    }

    bool addHits(const Hits &hits) {
        this->hits = hits;
        return true;
    }

    bool addTruth(const MCHits &mcHits, const std::string truth = "") {
        this->mcHits = mcHits;
        this->mcTruth = truth;
        return true;
    }

  private:
    template <typename T> std::string jsonify(const std::vector<T> &data, const std::string &label);
    template <typename T> std::string jsonify(const T &data, const std::string &label);

    httplib::Server server;

    std::optional<DetectorGeometry> geometry;
    std::optional<Hits> hits;
    std::optional<MCHits> mcHits;
    std::optional<std::string> mcTruth;
};

template <typename T>
inline std::string HttpEventDisplayServer::jsonify(const std::vector<T> &data, const std::string &label) {

    if (data.size() == 0) {
        return "";
    }

    std::stringstream json_string;
    json_string << "{"
                << "\"" << label << "\": [";

    for (const auto &dataPoint : data) {
        json_string << dataPoint << ",";
    }

    // Move the stringstream write head back one char,
    // removing the trailing comma, then close the JSON.
    json_string.seekp(-1, json_string.cur);
    json_string << "]}";

    return json_string.str();
}

template <typename T> inline std::string HttpEventDisplayServer::jsonify(const T &data, const std::string &label) {
    return HttpEventDisplayServer::jsonify(std::vector<T>({data}), label);
}

inline void HttpEventDisplayServer::startServer() {
    using namespace httplib;

    this->server.Get("/hello_world",
                     [](const Request &, Response &res) { res.set_content("Hello, World!", "text/plain"); });

    // Simple commands to return the currently understood server state.
    this->server.Get("/hits", [&](const Request &, Response &res) {
        res.set_content(this->jsonify<Hit>(this->hits.value(), "hits"), "application/json");
    });
    this->server.Get("/mc_hits", [&](const Request &, Response &res) {
        res.set_content(this->jsonify<MCHit>(this->mcHits.value(), "mcHits"), "application/json");
    });
    this->server.Get("/geometry", [&](const Request &, Response &res) {
        res.set_content(this->jsonify<DetectorGeometry>(this->geometry.value(), "geometry"), "application/json");
    });

    this->server.listen("localhost", CPPHEP_EVD_PORT);
}

}; // namespace HepEVD

#endif // CPPHEP_EVD_H
