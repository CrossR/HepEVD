//
// Particles
//
// Represent a particle, and any associated children.

#ifndef HEP_EVD_HIERARCHIES_H
#define HEP_EVD_HIERARCHIES_H

#include "hits.h"
#include "marker.h"
#include "utils.h"

#include "extern/json.hpp"
using json = nlohmann::json;
#include "extern/rapidjson/stringbuffer.h"
#include "extern/rapidjson/writer.h"

#include <map>
#include <vector>

namespace HepEVD {

enum InteractionType { BEAM, COSMIC, NEUTRINO, OTHER };
NLOHMANN_JSON_SERIALIZE_ENUM(InteractionType,
                             {{BEAM, "Beam"}, {COSMIC, "Cosmic"}, {NEUTRINO, "Neutrino"}, {OTHER, "Other"}});

enum RenderType { PARTICLE, TRACK, SHOWER };
NLOHMANN_JSON_SERIALIZE_ENUM(RenderType, {{PARTICLE, "Particle"}, {TRACK, "Track"}, {SHOWER, "Shower"}});

// Represent a single particle in the event.
class Particle {
  public:
    Particle() : m_hits({}), m_label(""), m_id(""), m_parentID(""), m_childIDs({}) {}
    Particle(const Hits &hits, const std::string id = "", const std::string &label = "")
        : m_hits(hits), m_label(label), m_id(id), m_parentID(""), m_childIDs({}) {}

    double getEnergy() const {
        double energy = 0.0;
        for (const auto &hit : this->m_hits)
            energy += hit->getEnergy();
        return energy;
    }

    unsigned int getNHits() const { return this->m_hits.size(); }
    std::string getLabel() const { return this->m_label; }
    std::string getID() const { return this->m_id; }

    void setVertices(const Markers &vertices) {
        // Check that the markers are all Point-type.
        for (const auto &marker : vertices) {
            if (!std::holds_alternative<Point>(marker)) {
                throw std::invalid_argument("All markers must be of type Point");
            }
        }

        this->m_vertices = vertices;
    }
    Markers getVertices() const { return this->m_vertices; }

    void setParentID(const std::string parentID) { this->m_parentID = parentID; }

    void setChildIDs(const std::vector<std::string> &childIDs) { this->m_childIDs = childIDs; }
    void addChild(const std::string childID) { this->m_childIDs.push_back(childID); }

    void setPrimary(bool primary) { this->m_primary = primary; }
    bool getPrimary() const { return this->m_primary; }

    void setInteractionType(InteractionType interactionType) { this->m_interactionType = interactionType; }
    InteractionType getInteractionType() const { return this->m_interactionType; }

    void setRenderType(RenderType renderType) { this->m_renderType = renderType; }
    RenderType getRenderType() const { return this->m_renderType; }

    // RapidJSON serialization, which is faster than nlohmann::json.
    // This is important for the potentially large number of particles.
    template <typename WriterType> void writeJson(WriterType &writer) const {
        writer.StartObject();

        writer.Key("id");
        writer.String(m_id.c_str(), static_cast<rapidjson::SizeType>(m_id.length()));

        writer.Key("label");
        writer.String(m_label.c_str(), static_cast<rapidjson::SizeType>(m_label.length()));

        writer.Key("hits");
        writer.StartArray();
        for (const auto &hit : m_hits) {
            hit->writeJson(writer);
        }
        writer.EndArray();

        writer.Key("vertices");
        writer.StartArray();
        for (const auto &marker : m_vertices) {
            const Point &point = std::get<Point>(marker);
            point.writeJson(writer);
        }
        writer.EndArray();

        writer.Key("primary");
        writer.Bool(m_primary);

        writer.Key("interactionType");
        switch (m_interactionType) {
        case BEAM:
            writer.String("Beam");
            break;
        case COSMIC:
            writer.String("Cosmic");
            break;
        case NEUTRINO:
            writer.String("Neutrino");
            break;
        case OTHER:
            writer.String("Other");
            break;
        }

        writer.Key("renderType");
        switch (m_renderType) {
        case PARTICLE:
            writer.String("Particle");
            break;
        case TRACK:
            writer.String("Track");
            break;
        case SHOWER:
            writer.String("Shower");
            break;
        }

        writer.Key("parentID");
        writer.String(m_parentID.c_str(), static_cast<rapidjson::SizeType>(m_parentID.length()));

        writer.Key("childIDs");
        writer.StartArray();
        for (const auto &childID : m_childIDs) {
            writer.String(childID.c_str(), static_cast<rapidjson::SizeType>(childID.length()));
        }
        writer.EndArray();

        writer.EndObject();
    }

    // Define to_json and from_json for Particle.
    friend void to_json(json &j, const Particle &particle) {
        j["id"] = particle.m_id;
        j["label"] = particle.m_label;
        j["hits"] = particle.m_hits;
        j["vertices"] = particle.m_vertices;
        j["primary"] = particle.m_primary;
        j["interactionType"] = particle.m_interactionType;
        j["renderType"] = particle.m_renderType;
        j["parentID"] = particle.m_parentID;
        j["childIDs"] = particle.m_childIDs;
    }

    friend void from_json(const json &j, Particle &particle) {
        j.at("id").get_to(particle.m_id);
        j.at("label").get_to(particle.m_label);
        j.at("hits").get_to(particle.m_hits);
        j.at("vertices").get_to(particle.m_vertices);
        j.at("primary").get_to(particle.m_primary);
        j.at("interactionType").get_to(particle.m_interactionType);
        j.at("renderType").get_to(particle.m_renderType);
        j.at("parentID").get_to(particle.m_parentID);
        j.at("childIDs").get_to(particle.m_childIDs);
    }

  private:
    Hits m_hits;
    Markers m_vertices;
    std::string m_label;
    std::string m_id;

    bool m_primary;
    InteractionType m_interactionType;

    // How to render the particle.
    // Default is PARTICLE, but can also be TRACK, SHOWER.
    RenderType m_renderType = RenderType::PARTICLE;

    // Lets just assume that the IDs are enough, and the list of particles
    // is available by the consumer (i.e. the Web UI). Less contained (could
    // instead have pointers to the parent/children), but easier to manage and
    // easier to serialise.
    std::string m_parentID;
    std::vector<std::string> m_childIDs;
};
using Particles = std::vector<Particle *>;

inline static void to_json(json &j, const Particles &particles) {

    if (particles.size() == 0) {
        j = json::array();
        return;
    }

    for (const auto &particle : particles) {
        j.push_back(*particle);
    }
}

inline static void from_json(const json &j, Particles &particles) {
    for (const auto &particle : j) {
        particles.push_back(new Particle(particle));
    }
}

}; // namespace HepEVD

#endif // HEP_EVD_HIERARCHIES_H
