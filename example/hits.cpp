#include "hep_evd.hpp"

#include <random>

int main(void) {

    HepEVD::HttpEventDisplayServer server;
    HepEVD::Hits hits;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-300, 300);

    for (unsigned int i = 0; i < 100; ++i) {
        HepEVD::Hit hit({dis(gen), dis(gen), dis(gen)});
        hits.push_back(hit);
    }

    server.addHits(hits);

    server.startServer();
    return 0;
}
