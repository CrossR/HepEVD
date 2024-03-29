//
// Helper Base
//
// Using the HepEVD server can be a pain, especially when you're trying to
// use it in multiple places. This file provides a set of helper functions
// to make it easier to use the HepEVD server, by providing some global state
// objects and functions to make it easier to use.

#ifndef HEP_EVD_BASE_HELPER_H
#define HEP_EVD_BASE_HELPER_H

#include <functional>
#include <map>
#include <string>

// Local Includes
#include "geometry.h"
#include "hits.h"
#include "particle.h"
#include "server.h"
#include "utils.h"

namespace HepEVD {

// Global Server Instance + HitMap
// This means we don't have to worry about setting it up,
// or awkwardness around using across multiple functions.
inline HepEVDServer *hepEVDServer;
inline bool verboseLogging = false;

// Helper functions to get the server or update the logging level.
static HepEVDServer *getServer() { return hepEVDServer; }
static void setVerboseLogging(const bool logging) { verboseLogging = logging; }

// We need to keep track of the hits we've added to the server, so we can
// map to them from whatever external data type we are using.
// Define a HitMapManager to keep track of the hits we've added, so
// that we can clear the maps when we need to reset the server / state.
class HitMapManager {
public:
    void clear() {
        for (auto &clearFunction : clearFunctions) {
            clearFunction();
        }
    }
    void registerClearFunction(std::function<void()> clearFunction) { clearFunctions.push_back(clearFunction); }
private:
    std::vector<std::function<void()>> clearFunctions;
};
inline HitMapManager hepEvdHitMapManager;

// This should be called very early on, to ensure the maps are correctly
// cleared. This is important to avoid memory leaks.
static void registerClearFunction(std::function<void()> clearFunction) {
    hepEvdHitMapManager.registerClearFunction(clearFunction);
}

// Check if the server is initialised, and if not, print a message.
// This means we can be certain that the server is set up before we
// try to use it.
static bool isServerInitialised() {
    const bool isInit(hepEVDServer != nullptr && hepEVDServer->isInitialised());

    if (!isInit && verboseLogging) {
        std::cout << "HepEVD Server is not initialised!" << std::endl;
        std::cout << "Please set the HepEVD geometry first!" << std::endl;
        std::cout << "This should be done before any other calls to the event display." << std::endl;
    }

    return isInit;
}

static void startServer(const int startState = -1) {
    if (!isServerInitialised())
        return;

    if (verboseLogging) {
        std::cout << "HepEVD: There are " << hepEVDServer->getHits().size() << " hits registered!" << std::endl;
        std::cout << "HepEVD: There are " << hepEVDServer->getMCHits().size() << " MC hits registered!" << std::endl;
        std::cout << "HepEVD: There are " << hepEVDServer->getParticles().size() << " particles registered!"
                  << std::endl;
        std::cout << "HepEVD: There are " << hepEVDServer->getMarkers().size() << " markers registered!" << std::endl;
    }

    if (startState != -1)
        hepEVDServer->swapEventState(startState);
    else if (hepEVDServer->getState()->isEmpty())
        hepEVDServer->previousEventState();

    hepEVDServer->startServer();
}

static void saveState(const std::string stateName, const int minSize = -1, const bool clearOnShow = true) {

    if (!isServerInitialised())
        return;

    // Set the name of the current state...
    hepEVDServer->setName(stateName);
    bool shouldIncState = true;

    // If prior to adding the new state, the size of the current state was
    // greater than the minimum size, then start the server.
    //
    // This is useful so you can save states as you go, but start the
    // server after a certain number of states have been saved.
    if (minSize != -1 && hepEVDServer->getNumberOfEventStates() >= minSize) {
        hepEVDServer->startServer();

        if (clearOnShow) {
            hepEVDServer->resetServer();
            shouldIncState = false;
        }
    }

    // Finally, start a new state and make sure it's the current one.
    if (shouldIncState) {
        hepEVDServer->addEventState();
        hepEVDServer->nextEventState();
    }

    hepEvdHitMapManager.clear();
}

static void resetServer(const bool resetGeo = false) {
    if (!isServerInitialised())
        return;

    hepEVDServer->resetServer(resetGeo);
    hepEvdHitMapManager.clear();
}

static void addMarkers(const Markers &markers) {
    if (!isServerInitialised())
        return;

    hepEVDServer->addMarkers(markers);
}

}; // namespace HepEVD

#endif // HEP_EVD_BASE_HELPER_H
