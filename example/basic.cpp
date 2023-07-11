#include "hep_evd.h"

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

    std::uniform_real_distribution<float> disProb(0, 1);

    for (unsigned int i = 0; i < 10000; ++i) {

        const double x = disX(gen);
        const double y = disY(gen);
        const double z = disZ(gen);
        const double e = x + y + z;

        Hit* hit = new Hit({x, y, z}, e);
        MCHit* mcHit = new MCHit({disX(gen), disY(gen), disZ(gen)});

        std::map<std::string, double> properties;

        if (x < -250.0)
            properties["Left"] = 1.0f;
        if (x > 250.0)
            properties["Right"] = 1.0f;

        if (y < -500.0)
            properties["Bottom"] = 1.0f;
        if (y > 500.0)
            properties["Top"] = 1.0f;

        if (z < 100.0)
            properties["Front"] = 1.0f;
        if (z > 1200.0)
            properties["Back"] = 1.0f;

        hit->addProperties(properties);
        hits.push_back(hit);
        mcHits.push_back(mcHit);
    }

    HepEVDServer server(
        DetectorGeometry(vols), hits, mcHits
    );

    server.startServer();
    return 0;
}
