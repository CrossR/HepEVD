//
// Top level HepEVD server.
//
// Once constructed and given a detector geometry and hits,
// running the server will spin up the event display on
// localhost::HEP_EVD_PORT, or use an environment variable,
// if present.

#ifndef HEP_EVD_SERVER_H
#define HEP_EVD_SERVER_H

#include "config.h"
#include "geometry.h"
#include "hits.h"
#include "marker.h"
#include "particle.h"
#include "state.h"

#include "extern/httplib.h"

#include "extern/json.hpp"
using json = nlohmann::json;

namespace HepEVD {

class HepEVDServer {
  public:
    HepEVDServer() : geometry({}), eventStates() {}
    HepEVDServer(const DetectorGeometry &geo = {}, const Hits &hits = {}, const MCHits &mc = {})
        : geometry(geo), eventStates() {
        currentState = 0;
        eventStates[currentState] = EventState("Initial", {}, hits, mc, {}, "");
    }
    HepEVDServer(std::string name, const DetectorGeometry &geo = {}, const Hits &hits = {}, const MCHits &mc = {})
        : geometry(geo), eventStates() {
        currentState = 0;
        eventStates[currentState] = EventState(name, {}, hits, mc, {}, "");
    }

    ~HepEVDServer() { this->eventStates.clear(); }

    // Check if the server is initialised.
    // Technically, all we need is a geometry.
    bool isInitialised() { return this->geometry.size() > 0; }

    // Reset the sever.
    // Hits and markers etc. should be cleared, but its unlikely
    // the geometry needs it, so make that optional.
    void resetServer(const bool resetGeo = false) {

        this->eventStates.clear();
        this->currentState = 0;
        this->eventStates[this->currentState] = EventState("Initial", {}, {}, {}, {}, "");

        if (resetGeo)
            this->geometry.clear();

        return;
    }

    // Add a new event state.
    // This will be used to store multiple events, or multiple
    // parts of the same event.
    EventState *getState() { return &this->eventStates[this->currentState]; }
    void addEventState(std::string name = "", Particles particles = {}, Hits hits = {}, MCHits mcHits = {},
                       Markers markers = {}, std::string mcTruth = "") {
        this->eventStates[this->eventStates.size()] = EventState(name, particles, hits, mcHits, markers, mcTruth);
    }

    // Swap to a different event state.
    void swapEventState(const int state) {
        if (this->eventStates.find(state) != this->eventStates.end())
            this->currentState = state;
    }
    void swapEventState(const std::string name) {
        for (auto &state : this->eventStates) {
            if (state.second.name == name) {
                this->currentState = state.first;
                return;
            }
        }
    }
    void nextEventState() {
        if (this->currentState < this->eventStates.size() - 1)
            this->currentState++;
    }
    void previousEventState() {
        if (this->currentState > 0)
            this->currentState--;
    }
    int getNumberOfEventStates() { return this->eventStates.size(); }
    void setName(const std::string name) { this->getState()->name = name; }

    // Start the event display server, blocking until exit is called by the
    // server.
    void startServer();

    // Pass over the required event information.
    // TODO: Verify the information passed over.
    bool addHits(const Hits &inputHits) {

        if (this->getState()->hits.size() == 0) {
            this->getState()->hits = inputHits;
            return true;
        }

        Hits newHits = this->getState()->hits;
        newHits.insert(newHits.end(), inputHits.begin(), inputHits.end());
        this->getState()->hits = newHits;

        return true;
    }
    Hits getHits() { return this->getState()->hits; }

    bool addMarkers(const Markers &inputMarkers) {

        if (this->getState()->markers.size() == 0) {
            this->getState()->markers = inputMarkers;
            return true;
        }

        Markers newMarkers = this->getState()->markers;
        newMarkers.insert(newMarkers.end(), inputMarkers.begin(), inputMarkers.end());
        this->getState()->markers = newMarkers;

        return true;
    }
    Markers getMarkers() { return this->getState()->markers; }

    bool addMCHits(const MCHits &inputMCHits) {
        if (this->getState()->mcHits.size() == 0) {
            this->getState()->mcHits = inputMCHits;
            return true;
        }

        MCHits newHits = this->getState()->mcHits;
        newHits.insert(newHits.end(), inputMCHits.begin(), inputMCHits.end());
        this->getState()->mcHits = newHits;

        return true;
    }
    MCHits getMCHits() { return this->getState()->mcHits; }

    bool addParticles(const Particles &inputParticles) {
        if (this->getState()->particles.size() == 0) {
            this->getState()->particles = inputParticles;
            return true;
        }

        Particles newParticles = this->getState()->particles;
        newParticles.insert(newParticles.end(), inputParticles.begin(), inputParticles.end());
        this->getState()->particles = newParticles;

        return true;
    }
    Particles getParticles() { return this->getState()->particles; }

  private:

    httplib::Server server;

    DetectorGeometry geometry;
    unsigned int currentState;
    EventStates eventStates;
};

// Run the actual server, spinning up the API endpoints and serving the
// HTML/JS required for the event display.
inline void HepEVDServer::startServer() {
    using namespace httplib;

    if (std::getenv("HEPEVD_NO_DISPLAY"))
        return;

    // Every endpoint has two parts:
    // 1. Get: Access the data.
    // 2. Post: Update the data.

    // First, the actual event hits.
    this->server.Get("/hits", [&](const Request &, Response &res) {
        res.set_content(json(this->getHits()).dump(), "application/json");
    });
    this->server.Post("/hits", [&](const Request &req, Response &res) {
        try {
            this->addHits(json::parse(req.body));
            res.set_content("OK", "text/plain");
        } catch (const std::exception &e) {
            res.set_content("Error: " + std::string(e.what()), "text/plain");
        }
    });

    // Next, the MC truth hits.
    this->server.Get("/mcHits", [&](const Request &, Response &res) {
        res.set_content(json(this->getMCHits()).dump(), "application/json");
    });
    this->server.Post("/mcHits", [&](const Request &req, Response &res) {
        try {
            this->addMCHits(json::parse(req.body));
            res.set_content("OK", "text/plain");
        } catch (const std::exception &e) {
            res.set_content("Error: " + std::string(e.what()), "text/plain");
        }
    });

    // Then any actual particles.
    this->server.Get("/particles", [&](const Request &, Response &res) {
        res.set_content(json(this->getParticles()).dump(), "application/json");
    });
    this->server.Post("/particles", [&](const Request &req, Response &res) {
        try {
            this->addParticles(json::parse(req.body));
            res.set_content("OK", "text/plain");
        } catch (const std::exception &e) {
            res.set_content("Error: " + std::string(e.what()), "text/plain");
        }
    });

    // Then, any markers (points, lines, rings, etc.)
    this->server.Get("/markers", [&](const Request &, Response &res) {
        res.set_content(json(this->getMarkers()).dump(), "application/json");
    });
    this->server.Post("/markers", [&](const Request &req, Response &res) {
        try {
            this->addMarkers(json::parse(req.body));
            res.set_content("OK", "text/plain");
        } catch (const std::exception &e) {
            res.set_content("Error: " + std::string(e.what()), "text/plain");
        }
    });

    // Finally, the detector geometry.
    this->server.Get("/geometry", [&](const Request &, Response &res) {
        res.set_content(json(this->geometry).dump(), "application/json");
    });
    this->server.Post("/geometry", [&](const Request &req, Response &res) {
        try {
            Volumes vols(json::parse(req.body));
            this->geometry = DetectorGeometry(vols);
            res.set_content("OK", "text/plain");
        } catch (const std::exception &e) {
            res.set_content("Error: " + std::string(e.what()), "text/plain");
        }
    });

    // Add a top level, dump everything endpoint.
    this->server.Get("/toJSON", [&](const Request &, Response &res) {
        json output;
        output["detectorGeometry"] = this->geometry;
        output["hits"] = this->getHits();
        output["mcHits"] = this->getMCHits();
        output["particles"] = this->getParticles();
        output["markers"] = this->getMarkers();
        res.set_content(output.dump(4), "application/json");
    });

    // State controls...
    this->server.Get("/allStateInfo", [&](const Request &, Response &res) {
        res.set_content(json(this->eventStates).dump(), "text/plain");
    });
    this->server.Get("/stateInfo", [&](const Request &, Response &res) {
        res.set_content(json(*this->getState()).dump(), "text/plain");
    });
    this->server.Get("/swap/id/:id", [&](const Request &req, Response &res) {
        try {
            this->swapEventState(std::stoi(req.path_params.at("id")));
            res.set_content("OK", "text/plain");
        } catch (const std::exception &e) {
            res.set_content("Error: " + std::string(e.what()), "text/plain");
        }
    });
    this->server.Get("/swap/name/:name", [&](const Request &req, Response &res) {
        try {
            this->swapEventState(req.path_params.at("name"));
            res.set_content("OK", "text/plain");
        } catch (const std::exception &e) {
            res.set_content("Error: " + std::string(e.what()), "text/plain");
        }
    });
    this->server.Get("/nextState", [&](const Request &, Response &res) {
        this->nextEventState();
        res.set_content("OK", "text/plain");
    });
    this->server.Get("/previousState", [&](const Request &, Response &res) {
        this->previousEventState();
        res.set_content("OK", "text/plain");
    });

    // Management controls...
    this->server.Get("/quit", [&](const Request &, Response &) { this->server.stop(); });

    // Finally, mount the www folder, which contains the actual HepEVD JS code.
    const std::string headerFilePath(__FILE__);
    const std::string includeFolder(headerFilePath.substr(0, headerFilePath.rfind("/")));
    this->server.set_mount_point("/", includeFolder + "/../web/");

    std::cout << "Starting a server on http://localhost:" << EVD_PORT() << "..." << std::endl;
    this->server.listen("localhost", EVD_PORT());
    std::cout << "Server closed, continuing..." << std::endl;
}

}; // namespace HepEVD

#endif // HEP_EVD_SERVER_H
