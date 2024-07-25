//
// Global function for the HepEVD Python Bindings
//

// Standard includes
#include <signal.h>

// Local Includes
#include "include/global.hpp"

namespace HepEVD_py {

// We want to catch SIGINT, SIGTERM and SIGKILL and shut down the server
// when that happens.
//
// But we also don't want to interfere with other signals, when the server
// is not running.
//
// So setup and teardown the signals here, around the server starting and finishing.
typedef void (*sighandler_t)(int);
std::vector<sighandler_t> catch_signals() {
    auto handler = [](int code) {
        if (HepEVD::hepEVDServer != nullptr) {
            std::cout << "HepEVD: Caught signal " << code << ", shutting down." << std::endl;
            HepEVD::hepEVDServer->stopServer();
        }
        exit(0);
    };

    std::vector<sighandler_t> oldHandlers;

    oldHandlers.push_back(signal(SIGINT, handler));
    oldHandlers.push_back(signal(SIGTERM, handler));
    oldHandlers.push_back(signal(SIGKILL, handler));

    return oldHandlers;
}

void revert_signals(std::vector<sighandler_t> oldHandlers) {
    signal(SIGINT, oldHandlers[0]);
    signal(SIGINT, oldHandlers[1]);
    signal(SIGINT, oldHandlers[2]);
}

void start_server(const int startState, const bool clearOnShow) {
    const auto oldHandlers = catch_signals();
    HepEVD::startServer(startState, clearOnShow);
    revert_signals(oldHandlers);
}

} // namespace HepEVD_py
