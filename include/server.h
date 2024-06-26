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
#include "utils.h"

#include "extern/httplib.h"

#include "extern/json.hpp"
using json = nlohmann::json;

namespace HepEVD {

class HepEVDServer {
  public:
    HepEVDServer() : geometry({}), eventStates() {}
    HepEVDServer(const DetectorGeometry &geo, const Hits &hits = {}, const MCHits &mc = {})
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

    // Less destructive clear function.
    // This will clear the hits, markers, particles, and MC hits,
    // but leave the geometry and event states alone.
    void clearState(const bool clearMCTruth = false) { this->eventStates[this->currentState].clear(clearMCTruth); }

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

    // Start/stop the event display server, blocking until exit is called by the
    // server.
    void startServer();
    void stopServer();

    // GUI configuration.
    GUIConfig *getConfig() { return &this->config; }

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

    void setMCTruth(const std::string mcTruth) { this->getState()->mcTruth = mcTruth; }

    // The MC truth is slightly unique, in that it should be the same across all states.
    // If there is multiple MC truths that aren't the same, either return the current one,
    // or return an empty string.
    // However, if there is just one in a single event state, just assume that's the one.
    std::string getMCTruth() {
        std::set<std::string> truths;

        for (auto &state : this->eventStates) {
            if (state.second.mcTruth.size() > 0)
                truths.insert(state.second.mcTruth);
        }

        if (truths.size() == 1)
            return truths.begin()->c_str();

        if (truths.size() > 1)
            return this->getState()->mcTruth;

        return "";
    }

  private:
    httplib::Server server;

    DetectorGeometry geometry;
    unsigned int currentState;
    EventStates eventStates;
    GUIConfig config;
};

// Run the actual server, spinning up the API endpoints and serving the
// HTML/JS required for the event display.
inline void HepEVDServer::startServer() {
    using namespace httplib;

    const char *noDisplay = std::getenv("HEP_EVD_NO_DISPLAY");
    if (noDisplay && std::string(noDisplay) == "1")
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

    // And the MC truth information.
    this->server.Get("/mcTruth",
                     [&](const Request &, Response &res) { res.set_content(this->getMCTruth(), "text/plain"); });

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
    this->server.Get("/stateToJson", [&](const Request &, Response &res) {
        json output;
        output["detectorGeometry"] = this->geometry;
        output["hits"] = this->getHits();
        output["mcHits"] = this->getMCHits();
        output["particles"] = this->getParticles();
        output["markers"] = this->getMarkers();
        output["stateInfo"] = *this->getState();
        output["config"] = *this->getConfig();
        res.set_content(output.dump(4), "application/json");
    });
    this->server.Get("/writeOutAllStates", [&](const Request &, Response &res) {
        // This is very different to the rest of the endpoints, as it needs to
        // write out JSON files for each state, rather than just returning the
        // data.

        // This was made because this is only really useful for either saving
        // the current event display state, or for producing data for the
        // GitHub pages version of the event display.

        // We want two things:
        // 1. A top level file that contains 3 things:
        //    - The detector geometry (static, so just define it once)
        //    - The number of states
        //    - An array of states names to a blank string.
        //      The blank string can be filled in later, if needed,
        //      with individual URLs to the state files.
        // 2. A file for each state, containing the full state information.

        // Populate the top level file.
        json infoFile;
        infoFile["detectorGeometry"] = this->geometry;
        infoFile["numberOfStates"] = this->eventStates.size();
        infoFile["config"] = *this->getConfig();
        infoFile["stateInfo"] = *this->getState();
        for (auto &state : this->eventStates) {
            json nameUrlPair({{"name", state.second.name}, {"url", ""}});
            infoFile["states"].push_back(nameUrlPair);
        }

        // Now write out the top level file.
        std::ofstream infoFileOut("eventDisplayInfo.json");
        infoFileOut << infoFile.dump(4);

        // Then populate the state files...
        for (unsigned int i = 0; i < this->eventStates.size(); i++) {
            json stateFile;
            EventState state = this->eventStates[i];

            if (state.isEmpty())
                continue;

            stateFile["name"] = state.name;
            stateFile["hits"] = state.hits;
            stateFile["mcHits"] = state.mcHits;
            stateFile["particles"] = state.particles;
            stateFile["markers"] = state.markers;
            stateFile["mcTruth"] = state.mcTruth;

            std::string formattedName = state.name;

            // Replace any spaces with underscores.
            std::replace(formattedName.begin(), formattedName.end(), ' ', '_');

            // Remove any non-alphanumeric characters.
            formattedName.erase(std::remove_if(formattedName.begin(), formattedName.end(),
                                               [](char c) { return !std::isalnum(c) && c != '_'; }),
                                formattedName.end());

            std::ofstream stateFileOut(std::to_string(i) + "_" + formattedName + ".json");
            stateFileOut << stateFile.dump();
        }

        // Alert the user to the files being written out.
        const auto currentDir = getCWD();
        res.set_content("Wrote out event display state files to " + currentDir, "text/plain");
    });

    // State controls...
    this->server.Get("/allStateInfo", [&](const Request &, Response &res) {
        res.set_content(json(this->eventStates).dump(), "application/json");
    });
    this->server.Get("/stateInfo", [&](const Request &, Response &res) {
        auto state = this->getState();
        const auto mcTruth = this->getMCTruth();

        // If the MC truth string and the MC truth in the state are different,
        // then we need to add the MC truth to the state.
        if (mcTruth.size() > 0 && state->mcTruth.size() == 0)
            state->mcTruth = mcTruth;

        res.set_content(json(*state).dump(), "application/json");
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
    this->server.Get("/config", [&](const Request &, Response &res) {
        res.set_content(json(*this->getConfig()).dump(), "application/json");
    });

    // Finally, mount the www folder, which contains the actual HepEVD JS code.
    this->server.set_mount_point("/", WEB_FOLDER());

    // Start the server.
    // Since the port may be in use, we need to keep trying until we find one that works.
    int port = EVD_PORT();
    while (portInUse(port)) {
        port++;
    }
    std::cout << "Starting HepEVD server on http://localhost:" << port << "..." << std::endl;
    this->server.listen("localhost", port);
    std::cout << "Server closed, continuing..." << std::endl;
}

inline void HepEVDServer::stopServer() { this->server.stop(); }

}; // namespace HepEVD

#endif // HEP_EVD_SERVER_H
