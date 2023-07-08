#include "hep_evd.hpp"

#include <random>

int main(void) {

    using namespace HepEVD;

    HttpEventDisplayServer server;
    Hits hits;

    BoxVolume volume1({-182.954544067, 0, 696.293762207}, 359.415008545, 1207.84753418, 1394.33996582);
    BoxVolume volume2({182.954544067, 0, 696.293762207}, 359.415008545, 1207.84753418, 1394.33996582);
    Volumes vols({&volume1, &volume2});

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<float> disX(-350, 350);
    std::uniform_real_distribution<float> disY(-350, 350);
    std::uniform_real_distribution<float> disZ(0, 1300);

    for (unsigned int i = 0; i < 100; ++i) {
        Hit hit({disX(gen), disY(gen), disZ(gen)});
        hits.push_back(hit);
    }

    server.addHits(hits);
    server.assignGeometry(DetectorGeometry(vols));

    server.startServer();
    return 0;
}
