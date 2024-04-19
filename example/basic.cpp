#include "hep_evd.h"

#include <random>

int main(void) {

    using namespace HepEVD;

    Hits hits;
    Hits eventTwoHits;
    Hits eventThreeHits;
    MCHits mcHits;

    BoxVolume volume1(Position({-182.954544067, 0, 696.293762207}), 359.415008545, 1207.84753418, 1394.33996582);
    BoxVolume volume2(Position({182.954544067, 0, 696.293762207}), 359.415008545, 1207.84753418, 1394.33996582);
    Volumes vols({volume1, volume2});

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<float> disX(-350, 350);
    std::uniform_real_distribution<float> disY(-600, 600);
    std::uniform_real_distribution<float> disZ(0, 1300);

    std::uniform_real_distribution<float> disProb(0, 1);

    const std::vector<double> pdgCodes({11, 13});
    std::uniform_int_distribution<> disPdg(0, 1);

    for (unsigned int i = 0; i < 25000; ++i) {

        const double x = disX(gen);
        const double y = disY(gen);
        const double z = disZ(gen);
        const double e = x + y + z;

        Hit *hit = new Hit({x, y, z}, e);
        MCHit *mcHit = new MCHit({disX(gen), disY(gen), disZ(gen)}, pdgCodes[disPdg(gen)], e);

        HitProperties properties;

        if (x < -250.0)
            properties[{"Left", PropertyType::CATEGORIC}] = 1.0f;
        if (x > 250.0)
            properties[{"Right", PropertyType::CATEGORIC}] = 1.0f;

        if (y < -500.0)
            properties[{"Bottom", PropertyType::CATEGORIC}] = 1.0f;
        if (y > 500.0)
            properties[{"Top", PropertyType::CATEGORIC}] = 1.0f;

        if (z < 100.0)
            properties[{"Front", PropertyType::CATEGORIC}] = 1.0f;
        if (z > 1200.0)
            properties[{"Back", PropertyType::CATEGORIC}] = 1.0f;

        // Add a continuous property, based on distance from the middle of the detector
        properties[{"Distance", PropertyType::NUMERIC}] = std::abs(x) + std::abs(y) + std::abs(z);

        hit->addProperties(properties);
        hits.push_back(hit);
        mcHits.push_back(mcHit);

        // Setup a second event, where the hits are only those in the left volume
        if (x < 0.0) {
            eventTwoHits.push_back(hit);
        }

        // Setup a third event, where the hits are only those in the right volume
        if (x > 0.0) {
            eventThreeHits.push_back(hit);
        }
    }

    // Repeat for the 2D views
    disX = std::uniform_real_distribution<float>(-350, 350);
    disZ = std::uniform_real_distribution<float>(0, 1300);
    std::array<HitType, 3> views({TWO_D_U, TWO_D_V, TWO_D_W});

    for (unsigned int i = 0; i < 3; ++i) {
        for (unsigned int j = 0; j < 5000; ++j) {
            const double x = disX(gen);
            const double z = disZ(gen);
            const double e = x + z;

            Hit *hit = new Hit({x, 0.f, z}, e);
            hit->setDim(TWO_D);
            hit->setHitType(views[i]);
            hits.push_back(hit);

            MCHit *mcHit = new MCHit({disX(gen), 0.f, disZ(gen)}, pdgCodes[disPdg(gen)], e);
            mcHit->setDim(TWO_D);
            mcHit->setHitType(views[i]);
            mcHits.push_back(mcHit);
        }
    }

    HepEVDServer server(DetectorGeometry(vols), hits, mcHits);

    Ring ring({0.0, 0.0, 0.0}, 1.0, 1.5);
    Point point({0.0, 0.0, 0.0});
    ring.setDim(TWO_D);
    point.setDim(TWO_D);
    server.addMarkers({ring, point});
    server.setMCTruth("\\nu_e \\rightarrow e^- + \\nu_e");

    server.addEventState("Second", {}, eventTwoHits, {}, {}, "");
    server.addEventState("Third", {}, eventThreeHits, {}, {}, "");

    server.startServer();
    return 0;
}
