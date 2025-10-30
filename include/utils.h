//
// Utilities
//
// General utils, used throughout the event display.

#ifndef HEP_EVD_POSITION_H
#define HEP_EVD_POSITION_H

#include "config.h"

#include "extern/httplib.h"
#include "extern/json.hpp"
using json = nlohmann::json;
#include "extern/rapidjson/stringbuffer.h"
#include "extern/rapidjson/writer.h"

#include <algorithm>
#include <array>
#include <functional>
#include <future>
#include <iterator>
#include <numeric>
#include <ostream>
#include <random>
#include <sstream>

namespace HepEVD {

using PosArray = std::array<double, 3>;

enum HitDimension { THREE_D, TWO_D };
NLOHMANN_JSON_SERIALIZE_ENUM(HitDimension, {{THREE_D, "3D"}, {TWO_D, "2D"}});

enum HitType { GENERAL, TWO_D_U, TWO_D_V, TWO_D_W };
NLOHMANN_JSON_SERIALIZE_ENUM(HitType,
                             {{GENERAL, "Hit"}, {TWO_D_U, "U View"}, {TWO_D_V, "V View"}, {TWO_D_W, "W View"}});

enum class PropertyType { CATEGORIC, NUMERIC };
NLOHMANN_JSON_SERIALIZE_ENUM(PropertyType,
                             {{PropertyType::CATEGORIC, "CATEGORIC"}, {PropertyType::NUMERIC, "NUMERIC"}});

// Forward declare enumToString, so we can use it in Position.
template <typename EnumType> static inline std::string enumToString(const EnumType &enumValue);

// Store a 3D position, and include a helper for JSON production.
class Position {

  public:
    Position() : x(0.0), y(0.0), z(0.0) {}
    Position(const PosArray &pos) : x(pos[0]), y(pos[1]), z(pos[2]) {}

    void setDim(HitDimension d) { dim = d; }
    void setHitType(HitType t) { hitType = t; }

    void setValue(const std::string &axes, double value) {
        if (axes == "x")
            this->x = value;
        else if (axes == "y")
            this->y = value;
        else if (axes == "z")
            this->z = value;
        else
            throw std::runtime_error("Invalid axes value for Position: " + axes);
    }

    // Implement comparison operators so we can sort Positions.
    bool operator<(const Position &other) const {
        if (this->x != other.x)
            return this->x < other.x;

        if (this->y != other.y)
            return this->y < other.y;

        return this->z < other.z;
    }

    // Implement equality operators so we can remove duplicates.
    bool operator==(const Position &other) const {
        return this->x == other.x && this->y == other.y && this->z == other.z;
    }

    // When converting to JSON, we want to convert 2D positions to use
    // XY, not XZ.
    template <typename WriterType> void writeJson(WriterType &writer) const {
        writer.StartObject();

        const bool is2D = this->dim == TWO_D;
        writer.Key("x");
        writer.Double(this->x);
        writer.Key("y");
        writer.Double(is2D ? this->z : this->y);
        writer.Key("z");
        writer.Double(is2D ? 0.0 : this->z);

        writer.Key("dim");
        writer.String(is2D ? "2D" : "3D");

        writer.Key("hitType");
        writer.String(enumToString(this->hitType).c_str(),
                      static_cast<rapidjson::SizeType>(enumToString(this->hitType).length()));

        writer.EndObject();
    }

    friend void to_json(json &j, const Position &pos) {
        if (pos.dim == THREE_D) {
            j = {{"x", pos.x}, {"y", pos.y}, {"z", pos.z}, {"dim", pos.dim}, {"hitType", pos.hitType}};
            return;
        }

        j = {{"x", pos.x}, {"y", pos.z}, {"z", 0.0}, {"dim", pos.dim}, {"hitType", pos.hitType}};
        return;
    }

    // That means we need to convert from XY to XZ when reading from JSON.
    friend void from_json(const json &j, Position &pos) {
        j.at("dim").get_to(pos.dim);
        j.at("hitType").get_to(pos.hitType);

        if (pos.dim == THREE_D) {
            j.at("x").get_to(pos.x);
            j.at("y").get_to(pos.y);
            j.at("z").get_to(pos.z);
        } else {
            j.at("x").get_to(pos.x);
            j.at("y").get_to(pos.z);
            pos.y = 0.0;
        }

        return;
    }

    // Allow for easy printing of positions.
    friend std::ostream &operator<<(std::ostream &os, const Position &pos) {
        os << "(" << pos.x << ", " << pos.y << ", " << pos.z << ")";
        return os;
    }

    double x, y, z;
    HitDimension dim = THREE_D;
    HitType hitType = GENERAL;
};

// General helper definition.
using Positions = std::vector<Position>;

// General templated utility function to POST data to a URL.
template <typename T> bool postData(const std::string &endPoint, const T &data) {
    const std::string server = "localhost:" + std::to_string(EVD_PORT());
    httplib::Client cli(server);
    auto res = cli.Post(endPoint, json(data).dump(), "application/json");
    return res.error() == httplib::Error::Success;
}

// Very basic UUID generator.
static std::string getUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    const char *v = "0123456789abcdef";
    const bool dash[] = {0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0};

    std::string res;
    for (int i = 0; i < 16; i++) {
        if (dash[i])
            res += "-";
        res += v[dis(gen)];
        res += v[dis(gen)];
    }

    return res;
}

static std::string getCWD() {
    char buff[FILENAME_MAX];

    if (getcwd(buff, FILENAME_MAX) != 0) {
        return std::string(buff);
    }

    return "";
}

// A basic map of PDG codes to particle names in LaTeX, and if they are visible
// in the detector.  This is used to provide a human-readable name for
// particles.
static const std::unordered_map<int, std::tuple<bool, std::string>> hepEVD_pdgMap = {
    {11, {true, "e^-"}},
    {12, {false, "\\nu_e"}},
    {-11, {true, "e^+"}},
    {-12, {false, "\\bar{\\nu}_e"}},
    {13, {true, "\\mu^-"}},
    {14, {false, "\\nu_\\mu"}},
    {-13, {true, "\\mu^+"}},
    {-14, {false, "\\bar{\\nu}_\\mu"}},
    {15, {true, "\\tau^-"}},
    {16, {false, "\\nu_\\tau"}},
    {-15, {true, "\\tau^+"}},
    {-16, {false, "\\bar{\\nu}_\\tau"}},
    {22, {true, "\\gamma"}},
    {111, {true, "\\pi^0"}},
    {211, {true, "\\pi^+"}},
    {-211, {true, "\\pi^-"}},
    {2212, {true, "p"}},
    {2112, {false, "n"}},
    {1000180390, {true, "^{39}\\mathrm{Ar}"}},
    {1000180400, {true, "^{40}\\mathrm{Ar}"}}};

// Return true if the PDG should be included in any output text.
// This is a bit opinionated because:
//  - Includes neutrinos by default.
//  - Includes Pi0s, as its cleaner than including their decay products.
static inline bool pdgIsVisible(const int pdgCode, const bool includeNeutrino = true) {
    auto it = hepEVD_pdgMap.find(pdgCode);

    if (it == hepEVD_pdgMap.end() && pdgCode < 1000000000) {
        std::cout << "HepEVD: Unknown PDG code: " << pdgCode << std::endl;
        return true;
    } else if (pdgCode >= 1000000000) {
        return true;
    }

    if (std::abs(pdgCode) == 12 || std::abs(pdgCode) == 14 || std::abs(pdgCode) == 16)
        return includeNeutrino;

    return std::get<0>(it->second);
}

static inline std::string pdgToString(const int pdgCode, const double energy = -1.0, const std::string units = "GeV") {

    std::string res;
    auto it = hepEVD_pdgMap.find(pdgCode);

    if (it == hepEVD_pdgMap.end())
        res = "Unknown PDG: " + std::to_string(pdgCode);
    else
        res = std::get<1>(it->second);

    if (energy > 0.0)
        res += " (\\text{" + std::to_string(energy) + " " + units + "})";

    return res;
}

// Check if a port is in use.
// This is achieved by calling out to netcat, since the
// alternatives can be a bit platform dependent / annoying.
static inline bool portInUse(const int port) {
    const std::string cmd("nc -z localhost " + std::to_string(port) + " 2> /dev/null");
    return std::system(cmd.c_str()) == 0;
}

// Processes elements of a container in parallel using multiple threads.
//
// Splits the container into chunks and applies a processing function to each chunk
// concurrently. Returns a vector containing the result produced by each thread.
template <typename Container, typename Func,
          typename ResultType = typename std::invoke_result<Func, typename Container::const_iterator,
                                                              typename Container::const_iterator>::type>
std::vector<ResultType> parallel_process(const Container &container, Func process_chunk) {

    size_t num_items = container.size();
    std::vector<ResultType> all_results;

    if (num_items == 0)
        return all_results;

    // Get the number of threads to use.
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0)
        num_threads = 4;

    // Avoid over-threading for small workloads.
    size_t min_items_per_thread = 50;
    if (num_items < num_threads * min_items_per_thread) {
        num_threads = 1;
    }

    // Fallback, single threaded case.
    if (num_threads == 1) {
        all_results.push_back(process_chunk(container.cbegin(), container.cend()));
        return all_results;
    }

    // Regular multi-threaded case.
    std::vector<std::future<ResultType>> futures;
    size_t items_per_thread = static_cast<size_t>(std::ceil(static_cast<double>(num_items) / num_threads));
    auto it_begin = container.cbegin();

    for (unsigned int i = 0; i < num_threads; ++i) {
        size_t start_index = i * items_per_thread;
        if (start_index >= num_items)
            break;

        size_t current_chunk_size = std::min(items_per_thread, num_items - start_index);
        auto chunk_begin = std::next(it_begin, start_index);
        auto chunk_end = std::next(chunk_begin, current_chunk_size);

        // Launch async task, passing the process_chunk function
        futures.push_back(std::async(std::launch::async, process_chunk, chunk_begin, chunk_end));
    }

    // Collect results from all threads
    all_results.reserve(futures.size());
    for (auto &fut : futures) {
        all_results.push_back(fut.get());
    }

    return all_results;
}

// Given a container, process its element into a JSON string, using multiple threads.
template <typename Container> std::string parallel_to_json_array(const Container &container) {
    // Define a lambda to process one chunk and return a JSON array string
    auto process_chunk = [&](typename Container::const_iterator begin,
                             typename Container::const_iterator end) -> std::string {
        rapidjson::StringBuffer s;
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);

        writer.StartArray();
        for (auto it = begin; it != end; ++it) {
            const auto *item_ptr = *it;
            item_ptr->writeJson(writer);
        }
        writer.EndArray();

        // This returns "[ {hitA}, {hitB} ]"
        return s.GetString();
    };

    // Call the parallel helper
    std::vector<std::string> json_array_fragments =
        parallel_process<Container, decltype(process_chunk), std::string>(container, process_chunk);

    // Now, lets combine the fragments into a single JSON array string.
    std::stringstream final_json_stream;
    final_json_stream << "[";

    bool first_fragment = true;
    for (const auto &fragment : json_array_fragments) {
        // Skip empty fragments (e.g., if a thread processed zero elems)
        // 2 here because we have at least "[]"
        if (fragment.length() <= 2)
            continue;

        // If this isn't the first fragment, we need a comma separator.
        if (!first_fragment)
            final_json_stream << ",";

        // Extract content between brackets: "[ {hitA}, {hitB} ]" -> " {hitA}, {hitB} "
        std::string_view fragment_content(fragment);
        fragment_content.remove_prefix(1);
        fragment_content.remove_suffix(1);

        final_json_stream << fragment_content;
        first_fragment = false;
    }

    // Close the array.
    final_json_stream << "]";

    return final_json_stream.str();
}

// Basic helper to convert an enum to a string for JSON output.
// Relies on nlohmann::json serialization of the enum.
template <typename EnumType> static inline std::string enumToString(const EnumType &enumValue) {
    json j = enumValue;
    return j.get<std::string>();
}

}; // namespace HepEVD

#endif // HEP_EVD_POSITION_H
