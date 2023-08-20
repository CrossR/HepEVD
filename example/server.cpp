// This is an example HepEVD client, that has no information in it at all,
// and instead is populated by external processes POSTing to it.

#include "hep_evd.h"

#include <random>

int main(void) {
    std::cout << "Starting HepEVD server..." << std::endl;
    std::cout << "Data needs to be POSTed to this client." << std::endl;

    HepEVD::HepEVDServer server;
    server.startServer();
    return 0;
}