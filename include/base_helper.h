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
inline bool hepEVDVerboseLogging = false;

// Helper functions to get the server or update the logging level.
static HepEVDServer *getServer() { return hepEVDServer; }
static void setVerboseLogging(const bool logging) { hepEVDVerboseLogging = logging; }

// Helper function to perform logging.
static void hepEVDLog(const std::string message) {
    if (hepEVDVerboseLogging)
        std::cout << "HepEVD INFO: " << message << std::endl;
}

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
    std::list<std::function<void()>> clearFunctions;
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
static bool isServerInitialised(const bool quiet = false) {
    const bool isInit(hepEVDServer != nullptr && hepEVDServer->isInitialised());

    if (!isInit && !quiet) {
        hepEVDLog("Quiet: " + std::to_string(quiet));
        hepEVDLog("HepEVD Server is not initialised!");
        hepEVDLog("Please set the HepEVD geometry first!");
        hepEVDLog("This should be done before any other calls to the event display.");
    }

    return isInit;
}

static void startServer(const int startState = -1, const bool clearOnShow = true) {
    if (!isServerInitialised())
        return;

    if (startState != -1)
        hepEVDServer->swapEventState(startState);
    else if (hepEVDServer->getState()->isEmpty())
        hepEVDServer->previousEventState();

    hepEVDLog("There are " + std::to_string(hepEVDServer->getHits().size()) + " hits registered!");
    hepEVDLog("There are " + std::to_string(hepEVDServer->getMCHits().size()) + " MC hits registered!");
    hepEVDLog("There are " + std::to_string(hepEVDServer->getParticles().size()) + " particles registered!");
    hepEVDLog("There are " + std::to_string(hepEVDServer->getMarkers().size()) + " markers registered!");

    hepEVDServer->startServer();

    if (clearOnShow) {
        hepEVDLog("Resetting the server...");
        hepEVDServer->resetServer();
    }
}

static void saveState(const std::string stateName, const int minSize = -1, const bool clearOnShow = true) {

    if (!isServerInitialised())
        return;

    hepEVDLog("Saving state: " + stateName);

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

    hepEVDLog("Resetting the server...");

    hepEVDServer->resetServer(resetGeo);
    hepEvdHitMapManager.clear();
}

static void clearState(const bool fullReset = false) {
    if (!isServerInitialised())
        return;

    hepEVDLog("Clearing server state...");

    // If we are doing a full reset, we also clear the MC truth.
    // Broadly, its assumed that clearState is for removing a
    // single state in an event and the MC truth is still valid.
    hepEVDServer->clearState(fullReset);
    hepEvdHitMapManager.clear();
}

static void addMarkers(const Markers &markers) {
    if (!isServerInitialised())
        return;

    hepEVDLog("Adding " + std::to_string(markers.size()) + " markers to the event display...");
    hepEVDServer->addMarkers(markers);
}

}; // namespace HepEVD

#endif // HEP_EVD_BASE_HELPER_H
