#include "hep_evd.hpp"

#include <random>

int main(void) {

    using namespace HepEVD;

    Hits hits;
    MCHits mcHits;

    BoxVolume volume1(
        Position({-182.954544067, 0, 696.293762207}),
        359.415008545, 1207.84753418, 1394.33996582
    );
    BoxVolume volume2(
        Position({182.954544067, 0, 696.293762207}),
        359.415008545, 1207.84753418, 1394.33996582
    );
    Volumes vols({&volume1, &volume2});

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<float> disX(-350, 350);
    std::uniform_real_distribution<float> disY(-600, 600);
    std::uniform_real_distribution<float> disZ(0, 1300);
    std::uniform_real_distribution<float> disE(0, 10);

    std::uniform_real_distribution<float> disProb(0, 1);

    for (unsigned int i = 0; i < 10000; ++i) {
        const double energy = disE(gen);
        Hit hit({disX(gen), disY(gen), disZ(gen)}, energy);
        MCHit mcHit({disX(gen), disY(gen), disZ(gen)});

        if (energy > 8.0) {
            hit.setLabel("High Energy");
        } else if (energy < 0.5) {
            hit.setLabel("Low Energy");
        } else {
            const double trackLike(disProb(gen));
            hit.setProperties({{"Track-Like", trackLike}, {"Shower-Like", 1 - trackLike}});
        }

        hits.push_back(hit);
        mcHits.push_back(mcHit);
    }

    HttpEventDisplayServer server(
        DetectorGeometry(vols), hits, mcHits
    );

    server.startServer();
    return 0;
}
