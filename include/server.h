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

#include <cstdint>
#include <fstream>

#include "extern/httplib.h"

#include "extern/json.hpp"
using json = nlohmann::json;

namespace HepEVD {

class HepEVDServer {
  public:
    HepEVDServer() : m_geometry({}), m_eventStates() {}
    HepEVDServer(const DetectorGeometry &geo, const Hits &hits = {}, const MCHits &mc = {})
        : m_geometry(geo), m_eventStates() {
        m_currentState = 0;
        m_eventStates[m_currentState] = EventState("Initial", {}, hits, mc, {}, {}, "");
    }
    HepEVDServer(std::string name, const DetectorGeometry &geo = {}, const Hits &hits = {}, const MCHits &mc = {})
        : m_geometry(geo), m_eventStates() {
        m_currentState = 0;
        m_eventStates[m_currentState] = EventState(name, {}, hits, mc, {}, {}, "");
    }

    ~HepEVDServer() { this->m_eventStates.clear(); }

    // Check if the server is initialised.
    // Technically, all we need is a geometry.
    bool isInitialised() { return this->m_geometry.size() > 0; }

    // Reset the sever.
    // Hits and markers etc. should be cleared, but its unlikely
    // the geometry needs it, so make that optional.
    void resetServer(const bool resetGeo = false) {

        this->m_eventStates.clear();
        this->m_currentState = 0;
        this->m_eventStates[this->m_currentState] = EventState("Initial", {}, {}, {}, {}, {}, "");

        if (resetGeo)
            this->m_geometry.clear();

        return;
    }

    // Less destructive clear function.
    // This will clear the hits, markers, particles, and MC hits,
    // but leave the geometry and event states alone.
    void clearState(const bool clearMCTruth = false) { this->m_eventStates[this->m_currentState].clear(clearMCTruth); }

    // Add a new event state.
    // This will be used to store multiple events, or multiple
    // parts of the same event.
    EventState *getState() { return &this->m_eventStates[this->m_currentState]; }
    void addEventState(std::string name = "", Particles particles = {}, Hits hits = {}, MCHits mcHits = {},
                       Markers markers = {}, Images images = {}, std::string mcTruth = "") {
        this->m_eventStates[this->m_eventStates.size()] =
            EventState(name, particles, hits, mcHits, markers, images, mcTruth);
    }

    // Swap to a different event state.
    void swapEventState(const int state) {
        if (this->m_eventStates.find(state) != this->m_eventStates.end())
            this->m_currentState = state;
    }
    void swapEventState(const std::string name) {
        for (auto &state : this->m_eventStates) {
            if (state.second.m_name == name) {
                this->m_currentState = state.first;
                return;
            }
        }
    }
    void nextEventState() {
        if (this->m_currentState < this->m_eventStates.size() - 1)
            this->m_currentState++;
    }
    void previousEventState() {
        if (this->m_currentState > 0)
            this->m_currentState--;
    }
    int getNumberOfEventStates() { return this->m_eventStates.size(); }
    void setName(const std::string name) { this->getState()->m_name = name; }

    // Start/stop the event display server, blocking until exit is called by the
    // server.
    void startServer();
    void stopServer();

    // GUI configuration.
    GUIConfig *getConfig() { return &this->m_config; }

    // Pass over the required event information.
    // TODO: Verify the information passed over.
    bool addHits(const Hits &inputHits) {

        if (this->getState()->m_hits.size() == 0) {
            this->getState()->m_hits = inputHits;
            return true;
        }

        Hits newHits = this->getState()->m_hits;
        newHits.insert(newHits.end(), inputHits.begin(), inputHits.end());
        this->getState()->m_hits = newHits;

        return true;
    }
    Hits getHits() { return this->getState()->m_hits; }

    bool addMarkers(const Markers &inputMarkers) {

        if (this->getState()->m_markers.size() == 0) {
            this->getState()->m_markers = inputMarkers;
            return true;
        }

        Markers newMarkers = this->getState()->m_markers;
        newMarkers.insert(newMarkers.end(), inputMarkers.begin(), inputMarkers.end());
        this->getState()->m_markers = newMarkers;

        return true;
    }
    Markers getMarkers() { return this->getState()->m_markers; }

    bool addImages(const Images &images) {

        if (this->getState()->m_images.size() == 0) {
            this->getState()->m_images = images;
            return true;
        }

        Images newImages = this->getState()->m_images;
        newImages.insert(newImages.end(), images.begin(), images.end());
        this->getState()->m_images = newImages;

        return true;
    }
    Images getImages() { return this->getState()->m_images; }

    bool addParticles(const Particles &inputParticles) {
        if (this->getState()->m_particles.size() == 0) {
            this->getState()->m_particles = inputParticles;
            return true;
        }

        Particles newParticles = this->getState()->m_particles;
        newParticles.insert(newParticles.end(), inputParticles.begin(), inputParticles.end());
        this->getState()->m_particles = newParticles;

        return true;
    }
    Particles getParticles() { return this->getState()->m_particles; }

    bool addMCHits(const MCHits &inputMCHits) {
        if (this->getState()->m_mcHits.size() == 0) {
            this->getState()->m_mcHits = inputMCHits;
            return true;
        }

        MCHits newHits = this->getState()->m_mcHits;
        newHits.insert(newHits.end(), inputMCHits.begin(), inputMCHits.end());
        this->getState()->m_mcHits = newHits;

        return true;
    }
    MCHits getMCHits() { return this->getState()->m_mcHits; }

    void setMCTruth(const std::string mcTruth) { this->getState()->m_mcTruth = mcTruth; }

    // The MC truth is slightly unique, in that it should be the same across all states.
    // If there is multiple MC truths that aren't the same, either return the current one,
    // or return an empty string.
    // However, if there is just one in a single event state, just assume that's the one.
    std::string getMCTruth() {
        std::set<std::string> truths;

        for (auto &state : this->m_eventStates) {
            if (state.second.m_mcTruth.size() > 0)
                truths.insert(state.second.m_mcTruth);
        }

        if (truths.size() == 1)
            return truths.begin()->c_str();

        if (truths.size() > 1)
            return this->getState()->m_mcTruth;

        return "";
    }

  private:
    httplib::Server m_server;

    DetectorGeometry m_geometry;
    unsigned int m_currentState;
    EventStates m_eventStates;
    GUIConfig m_config;
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
    this->m_server.Get("/hits", [&](const Request &, Response &res) {
        const std::vector<uint8_t> msgPackData = json::to_msgpack(this->getHits());
        res.set_content(reinterpret_cast<const char *>(msgPackData.data()), msgPackData.size(), "application/msgpack");
    });
    this->m_server.Post("/hits", [&](const Request &req, Response &res) {
        try {
            this->addHits(json::parse(req.body));
            res.set_content("OK", "text/plain");
        } catch (const std::exception &e) {
            res.set_content("Error: " + std::string(e.what()), "text/plain");
        }
    });

    // Next, the MC truth hits.
    this->m_server.Get("/mcHits", [&](const Request &, Response &res) {
        const std::vector<uint8_t> msgPackData = json::to_msgpack(this->getMCHits());
        res.set_content(reinterpret_cast<const char *>(msgPackData.data()), msgPackData.size(), "application/msgpack");
    });
    this->m_server.Post("/mcHits", [&](const Request &req, Response &res) {
        try {
            this->addMCHits(json::parse(req.body));
            res.set_content("OK", "text/plain");
        } catch (const std::exception &e) {
            res.set_content("Error: " + std::string(e.what()), "text/plain");
        }
    });

    // And the MC truth information.
    this->m_server.Get("/mcTruth",
                       [&](const Request &, Response &res) { res.set_content(this->getMCTruth(), "text/plain"); });

    // Then any actual particles.
    this->m_server.Get("/particles", [&](const Request &, Response &res) {
        const std::vector<uint8_t> msgPackData = json::to_msgpack(this->getParticles());
        res.set_content(reinterpret_cast<const char *>(msgPackData.data()), msgPackData.size(), "application/msgpack");
    });
    this->m_server.Post("/particles", [&](const Request &req, Response &res) {
        try {
            this->addParticles(json::parse(req.body));
            res.set_content("OK", "text/plain");
        } catch (const std::exception &e) {
            res.set_content("Error: " + std::string(e.what()), "text/plain");
        }
    });

    // Then, any markers (points, lines, rings, etc.)
    this->m_server.Get("/markers", [&](const Request &, Response &res) {
        const std::vector<uint8_t> msgPackData = json::to_msgpack(this->getMarkers());
        res.set_content(reinterpret_cast<const char *>(msgPackData.data()), msgPackData.size(), "application/msgpack");
    });
    this->m_server.Post("/markers", [&](const Request &req, Response &res) {
        try {
            this->addMarkers(json::parse(req.body));
            res.set_content("OK", "text/plain");
        } catch (const std::exception &e) {
            res.set_content("Error: " + std::string(e.what()), "text/plain");
        }
    });

    // Any supplied raw images
    this->m_server.Get("/images", [&](const Request &, Response &res) {
        const std::vector<uint8_t> msgPackData = json::to_msgpack(this->getImages());
        res.set_content(reinterpret_cast<const char *>(msgPackData.data()), msgPackData.size(), "application/msgpack");
    });
    this->m_server.Post("/images", [&](const Request &req, Response &res) {
        try {
            this->addImages(json::parse(req.body));
            res.set_content("OK", "text/plain");
        } catch (const std::exception &e) {
            res.set_content("Error: " + std::string(e.what()), "text/plain");
        }
    });

    // Finally, the detector geometry.
    this->m_server.Get("/geometry", [&](const Request &, Response &res) {
        const std::vector<uint8_t> msgPackData = json::to_msgpack(this->m_geometry);
        res.set_content(reinterpret_cast<const char *>(msgPackData.data()), msgPackData.size(), "application/msgpack");
    });
    this->m_server.Post("/geometry", [&](const Request &req, Response &res) {
        try {
            Volumes vols(json::parse(req.body));
            this->m_geometry = DetectorGeometry(vols);
            res.set_content("OK", "text/plain");
        } catch (const std::exception &e) {
            res.set_content("Error: " + std::string(e.what()), "text/plain");
        }
    });

    // Add a top level, dump everything endpoint.
    this->m_server.Get("/stateToJson", [&](const Request &, Response &res) {
        json output;
        output["detectorGeometry"] = this->m_geometry;
        output["hits"] = this->getHits();
        output["mcHits"] = this->getMCHits();
        output["particles"] = this->getParticles();
        output["markers"] = this->getMarkers();
        output["stateInfo"] = *this->getState();
        output["config"] = *this->getConfig();

        const std::vector<uint8_t> msgPackData = json::to_msgpack(output);
        res.set_content(reinterpret_cast<const char *>(msgPackData.data()), msgPackData.size(), "application/msgpack");
    });
    this->m_server.Get("/writeOutAllStates", [&](const Request &, Response &res) {
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
        infoFile["detectorGeometry"] = this->m_geometry;
        infoFile["config"] = *this->getConfig();
        infoFile["stateInfo"] = *this->getState();

        int numberOfStates(0);

        for (auto &state : this->m_eventStates) {

            if (state.second.isEmpty())
                continue;

            json nameUrlPair({{"name", state.second.m_name}, {"url", ""}});
            infoFile["states"].push_back(nameUrlPair);
            ++numberOfStates;
        }

        // Now write out the top level file.
        infoFile["numberOfStates"] = numberOfStates;
        std::ofstream infoFileOut("eventDisplayInfo.json");
        infoFileOut << infoFile.dump(4);

        // Then populate the state files...
        for (unsigned int i = 0; i < this->m_eventStates.size(); i++) {
            json stateFile;
            EventState state = this->m_eventStates[i];

            if (state.isEmpty())
                continue;

            stateFile["name"] = state.m_name;
            stateFile["hits"] = state.m_hits;
            stateFile["mcHits"] = state.m_mcHits;
            stateFile["particles"] = state.m_particles;
            stateFile["markers"] = state.m_markers;
            stateFile["mcTruth"] = state.m_mcTruth;

            std::string formattedName = state.m_name;

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
    this->m_server.Get("/allStateInfo", [&](const Request &, Response &res) {
        const std::vector<uint8_t> msgPackData = json::to_msgpack(this->m_eventStates);
        res.set_content(reinterpret_cast<const char *>(msgPackData.data()), msgPackData.size(), "application/msgpack");
    });
    this->m_server.Get("/stateInfo", [&](const Request &, Response &res) {
        auto state = this->getState();
        const auto mcTruth = this->getMCTruth();

        // If the MC truth string and the MC truth in the state are different,
        // then we need to add the MC truth to the state.
        if (mcTruth.size() > 0 && state->m_mcTruth.size() == 0)
            state->m_mcTruth = mcTruth;

        const std::vector<uint8_t> msgPackData = json::to_msgpack(*state);
        res.set_content(reinterpret_cast<const char *>(msgPackData.data()), msgPackData.size(), "application/msgpack");
    });
    this->m_server.Get("/swap/id/:id", [&](const Request &req, Response &res) {
        try {
            this->swapEventState(std::stoi(req.path_params.at("id")));
            res.set_content("OK", "text/plain");
        } catch (const std::exception &e) {
            res.set_content("Error: " + std::string(e.what()), "text/plain");
        }
    });
    this->m_server.Get("/swap/name/:name", [&](const Request &req, Response &res) {
        try {
            this->swapEventState(req.path_params.at("name"));
            res.set_content("OK", "text/plain");
        } catch (const std::exception &e) {
            res.set_content("Error: " + std::string(e.what()), "text/plain");
        }
    });
    this->m_server.Get("/nextState", [&](const Request &, Response &res) {
        this->nextEventState();
        res.set_content("OK", "text/plain");
    });
    this->m_server.Get("/previousState", [&](const Request &, Response &res) {
        this->previousEventState();
        res.set_content("OK", "text/plain");
    });

    // Management controls...
    this->m_server.Get("/quit", [&](const Request &, Response &) { this->m_server.stop(); });
    this->m_server.Get("/config", [&](const Request &, Response &res) {
        const std::vector<uint8_t> msgPackData = json::to_msgpack(*this->getConfig());
        res.set_content(reinterpret_cast<const char *>(msgPackData.data()), msgPackData.size(), "application/msgpack");
    });

    // Finally, mount the www folder, which contains the actual HepEVD JS code.
    this->m_server.set_mount_point("/", WEB_FOLDER());

    // Start the server.
    // Since the port may be in use, we need to keep trying until we find one that works.
    int port = EVD_PORT();
    while (portInUse(port)) {
        port++;
    }
    std::cout << "Starting HepEVD server on http://localhost:" << port << "..." << std::endl;
    this->m_server.listen("localhost", port);
    std::cout << "Server closed, continuing..." << std::endl;
}

inline void HepEVDServer::stopServer() { this->m_server.stop(); }

}; // namespace HepEVD

#endif // HEP_EVD_SERVER_H
