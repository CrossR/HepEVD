//
// Event state object.
//
// The state contains a snapshot of the current event, including the particles,
// hits, mcHits, markers, and mcTruth. Multiple states can be used to show
// different parts of the same event, or multiple events.

#ifndef HEP_EVD_STATE_H
#define HEP_EVD_STATE_H

#include "config.h"
#include "geometry.h"
#include "hits.h"
#include "marker.h"
#include "particle.h"

#include "extern/json.hpp"
using json = nlohmann::json;

namespace HepEVD {

// Top level state object, that contains everything about the current state of the
// event. This means we can more easily store multiple events or multiple
// parts of the same event.
class EventState {
  public:
    EventState() : name(""), particles(), hits(), mcHits(), markers(), mcTruth("") {}
    EventState(std::string name, Particles particles = {}, Hits hits = {}, MCHits mcHits = {}, Markers markers = {},
               std::string mcTruth = "")
        : name(name), particles(particles), hits(hits), mcHits(mcHits), markers(markers), mcTruth(mcTruth) {}

    bool isEmpty() {
        return name.size() == 0 && particles.empty() && hits.empty() && mcHits.empty() && markers.empty() &&
               mcTruth.size() == 0;
    }

    void clear(const bool resetMCTruth = false) {
        name = "";
        particles.clear();
        hits.clear();
        mcHits.clear();
        markers.clear();

        if (resetMCTruth)
            mcTruth = "";
    }

    // Only need a to JSON method, as we don't need to read in the state.
    // We also only want to pass the metadata, not the actual data.
    friend void to_json(json &j, const EventState &state) {
        j = {{"name", state.name},
             {"particles", state.particles.size()},
             {"hits", state.hits.size()},
             {"mcHits", state.mcHits.size()},
             {"markers", state.markers.size()},
             {"mcTruth", state.mcTruth}};
    }

    std::string name;
    Particles particles;
    Hits hits;
    MCHits mcHits;
    Markers markers;
    std::string mcTruth;
};

using EventStates = std::map<int, EventState>;

inline void to_json(json &j, const EventStates &states) {
    for (const auto &state : states) {
        // Don't include empty states.
        if (state.second.hits.size() == 0 && state.second.mcHits.size() == 0 && state.second.markers.size() == 0 &&
            state.second.particles.size() == 0)
            continue;
        j.push_back({{"id", state.first}, {"state", state.second}});
    }
}

// Configuration for the GUI, such as what to show and what not to show.
// This is only really useful when you want to overwrite some default settings in the GUI.
// For example, you may know you do not need a 2D view, so you can turn it off.
// Or edit default materials and colors.
class GUIConfig {
  public:
    GUIConfig() {}

    struct Material {
        std::string colour;
        float opacity = 0.0;
        float size = 0.0;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Material, colour, opacity, size);
    };

    bool show2D = true;
    bool show3D = true;

    Material hits;

    // Only need a to JSON method, as we don't need to read in the state.
    // We also only want to pass the metadata, not the actual data.
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(GUIConfig, showParticles, showHits, showMCHits, showMarkers, showMCParticles, show2D,
                                   show3D, renderGeometry, detector, hits);
};

} // namespace HepEVD

#endif
