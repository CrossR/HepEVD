#include "hep_evd.h"

#include <random>

int main(void) {

    using namespace HepEVD;

    Particles particles;
    Hits hits;
    MCHits mcHits;

    BoxVolume volume1(Position({-182.954544067, 0, 696.293762207}), 359.415008545, 1207.84753418, 1394.33996582);
    BoxVolume volume2(Position({182.954544067, 0, 696.293762207}), 359.415008545, 1207.84753418, 1394.33996582);
    Volumes vols({volume1, volume2});

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<float> disX(-5, 5);
    std::uniform_real_distribution<float> disY(-5, 5);
    std::uniform_real_distribution<float> disZ(-5, 5);

    // Find the 8 locations of the corners of the box.
    Positions corners;
    for (const auto &vol : {volume1, volume2}) {
        const Position &pos = vol.getCenter();
        const double xWidth = vol.getXWidth();
        const double yWidth = vol.getYWidth();
        const double zWidth = vol.getZWidth();

        corners.push_back(Position({pos.x - xWidth / 2.0, pos.y - yWidth / 2.0, pos.z - zWidth / 2.0}));
        corners.push_back(Position({pos.x - xWidth / 2.0, pos.y - yWidth / 2.0, pos.z + zWidth / 2.0}));
        corners.push_back(Position({pos.x - xWidth / 2.0, pos.y + yWidth / 2.0, pos.z - zWidth / 2.0}));
        corners.push_back(Position({pos.x - xWidth / 2.0, pos.y + yWidth / 2.0, pos.z + zWidth / 2.0}));
        corners.push_back(Position({pos.x + xWidth / 2.0, pos.y - yWidth / 2.0, pos.z - zWidth / 2.0}));
        corners.push_back(Position({pos.x + xWidth / 2.0, pos.y - yWidth / 2.0, pos.z + zWidth / 2.0}));
        corners.push_back(Position({pos.x + xWidth / 2.0, pos.y + yWidth / 2.0, pos.z - zWidth / 2.0}));
        corners.push_back(Position({pos.x + xWidth / 2.0, pos.y + yWidth / 2.0, pos.z + zWidth / 2.0}));
    }

    // Remove duplicates.
    std::sort(corners.begin(), corners.end());
    corners.erase(std::unique(corners.begin(), corners.end()), corners.end());

    // Now we have the corners, generate Particles at each corner.
    for (const auto &corner : corners) {

        Hits particleHits;

        for (unsigned int i = 0; i < 1000; ++i) {
            const double x = corner.x + disX(gen);
            const double y = corner.y + disY(gen);
            const double z = corner.z + disZ(gen);
            const double e = particles.size() + 1;

            Hit *hit = new Hit({x, y, z}, e);
            particleHits.push_back(hit);
        }

        Particle *particle = new Particle(particleHits, std::to_string(particles.size()), std::to_string(particles.size()));
        particles.push_back(particle);
    }

    HepEVDServer server(DetectorGeometry(vols), hits, mcHits);
    server.addParticles(particles);

    server.startServer();
    return 0;
}
