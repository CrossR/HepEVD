//
// Top level HepEVD server.
//
// Once constructed and given a detector geometry and hits,
// running the server will spin up the event display on
// localhost::HEP_EVD_PORT.

#ifndef HEP_EVD_SERVER_H
#define HEP_EVD_SERVER_H

#include "config.h"
#include "geometry.h"
#include "hits.h"
#include "marker.h"

#include "extern/httplib.h"

namespace HepEVD {

class HepEVDServer {
  public:
    HepEVDServer(const DetectorGeometry &geo) : geometry(geo), hits({}), mcHits({}), mcTruth("") {}
    HepEVDServer(const DetectorGeometry &geo, const Hits &hits) : geometry(geo), hits(hits), mcHits({}), mcTruth("") {}
    HepEVDServer(const DetectorGeometry &geo, const Hits &hits, const MCHits &mc)
        : geometry(geo), hits(hits), mcHits(mc), mcTruth("") {}

    ~HepEVDServer() {
        this->hits.clear();
        this->mcHits.clear();
    }

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

    bool addMarkers(const Markers &inputMarkers) {

        if (this->markers.size() == 0) {
            this->markers = inputMarkers;
            return true;
        }

        Markers newMarkers = this->markers;
        newMarkers.insert(newMarkers.end(), inputMarkers.begin(), inputMarkers.end());
        this->markers = newMarkers;

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
    Markers markers;
    std::string mcTruth;
};

// Produce a full JSON representation of the given items.
template <typename T> inline std::string HepEVDServer::jsonify(const std::vector<T> &data) {

    if (data.size() == 0) {
        return "[]";
    }

    std::stringstream json_string;
    json_string << "[";

    for (const auto &dataPoint : data) {
        if constexpr (std::is_pointer<T>::value)
            json_string << *dataPoint << ",";
        else
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
        res.set_content(this->jsonify<Hit *>(this->hits), "application/json");
    });
    this->server.Get("/mcHits", [&](const Request &, Response &res) {
        res.set_content(this->jsonify<MCHit *>(this->mcHits), "application/json");
    });
    this->server.Get("/markers", [&](const Request &, Response &res) {
        res.set_content(this->jsonify<Marker *>(this->markers), "application/json");
    });
    this->server.Get("/geometry", [&](const Request &, Response &res) {
        res.set_content(this->jsonify<DetectorGeometry>(this->geometry), "application/json");
    });

    // Management controls...
    this->server.Get("/quit", [&](const Request &, Response &) { this->server.stop(); });

    // Finally, mount the www folder, which contains the actual HepEVD JS code.
    const std::string headerFilePath(__FILE__);
    const std::string includeFolder(headerFilePath.substr(0, headerFilePath.rfind("/")));
    this->server.set_mount_point("/", includeFolder + "/../web/");

    std::cout << "Starting a server on http://localhost:" << HEP_EVD_PORT << "..." << std::endl;
    this->server.listen("localhost", HEP_EVD_PORT);
    std::cout << "Server closed, continuing..." << std::endl;
}

}; // namespace HepEVD

#endif // HEP_EVD_SERVER_H
