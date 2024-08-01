//
// Data Loader
//

import { addCitation } from "./ui.js";

// This is a global variable that will be used to store the state of the event
// display, when running on GitHub Pages.
//
// Is it ideal to have a global variable like this? Obviously not, but the
// alternative would make the code for loading data from a GitHub Gist URL
// much more spread out, and ingrained across multiple files. The event
// display is a development tool first and foremost, whereas the loading
// of data from a GitHub Gist URL is only really useful for the purposes of
// outreach or as a demo.
//
// When running on a server, this variable will be undefined.
// When running on GitHub Pages, this variable will be set to an object with
// the following structure:
//    - numberOfStates: The number of states in the event display.
//    - currentState: The current state of the event display.
//    - states: An array of objects, each of which contains the following:
//      - url: The URL for the state.
//      - description: A description of the state.
//    - detectorGeometry: The detector geometry for the event display.
// If the input github gist URL is a single JSON object, then this variable
// will be undefined.
export var hepEVD_GLOBAL_STATE = {
  _state: null,

  get initialised() {
    return this._state !== null;
  },

  get state() {
    return this._state;
  },

  set state(state) {
    this._state = state;
  },

  nextState() {
    if (!this.initialised) return;

    this.state.currentState =
      (this.state.currentState + 1) % this.state.states.length;
  },

  prevState() {
    if (!this.initialised) return;

    this.state.currentState =
      (this.state.currentState - 1 + this.state.states.length) %
      this.state.states.length;
  },
};

// Are we running on GitHub Pages?
export function isRunningOnGitHubPages() {
  return window.location.hostname.includes("github.io");
}

async function getDataWithProgress(url) {
  const response = await fetch(url);

  const reader = response.body.getReader();
  const contentLength = +response.headers.get("Content-Length");
  const loadingBar = document.getElementById("loading_bar_data");

  if (contentLength && contentLength > 250) {
    loadingBar.style.display = "block";
  } else {
    loadingBar.style.display = "none";
  }

  let receivedLength = 0;
  let chunks = [];

  while (true) {
    const { done, value } = await reader.read();

    if (done) {
      break;
    }

    chunks.push(value);
    receivedLength += value.length;

    const percent = Math.round((receivedLength / contentLength) * 100);

    if (contentLength && contentLength > 250)
      loadingBar.style.width = percent + "%";
  }

  let chunksAll = new Uint8Array(receivedLength);
  let position = 0;
  for (let chunk of chunks) {
    chunksAll.set(chunk, position);
    position += chunk.length;
  }

  let result = new TextDecoder("utf-8").decode(chunksAll);

  if (contentLength && contentLength > 250) loadingBar.style.display = "none";

  return JSON.parse(result);
}

// Simple function to pull down all data from the server.
async function loadServerData() {
  let detectorGeometry = getDataWithProgress("geometry");
  let hits = getDataWithProgress("hits");
  let mcHits = getDataWithProgress("mcHits");
  let markers = getDataWithProgress("markers");
  let particles = getDataWithProgress("particles");
  let images = getDataWithProgress("images");
  let stateInfo = getDataWithProgress("stateInfo");
  let config = getDataWithProgress("config");

  // Wait for all the data to be loaded.
  [
    hits,
    mcHits,
    markers,
    particles,
    images,
    detectorGeometry,
    stateInfo,
    config,
  ] = await Promise.all([
    hits,
    mcHits,
    markers,
    particles,
    images,
    detectorGeometry,
    stateInfo,
    config,
  ]);

  return {
    hits: hits,
    mcHits: mcHits,
    markers: markers,
    particles: particles,
    images: images,
    detectorGeometry: detectorGeometry,
    stateInfo: stateInfo,
    config: config,
  };
}

/**
 * Updates the external data by fetching new data from the specified URL.
 *
 * @returns {Promise<Object>} An object containing the updated data, including hits, mcHits, markers, particles, and detectorGeometry.
 */
async function updateExternalData() {
  const newDataUrl =
    hepEVD_GLOBAL_STATE.state.states[hepEVD_GLOBAL_STATE.state.currentState]
      .url;
  const newStateData = await getDataWithProgress(newDataUrl);

  return {
    hits: newStateData.hits,
    mcHits: newStateData.mcHits,
    markers: newStateData.markers,
    particles: newStateData.particles,
    images: newStateData.images || [],
    detectorGeometry: hepEVD_GLOBAL_STATE.state.detectorGeometry,
    stateInfo: newStateData.stateInfo || { mcTruth: "" },
    config: hepEVD_GLOBAL_STATE.state.config || {},
  };
}

// Load data from a GitHub Gist URL.
async function loadExternalData(url) {
  // Now, request the data from the supplied GitHub Gist URL.
  // First, just check tha the URL is valid and is a raw URL, not a
  // link to the GitHub page.
  if (!url.includes("gist.githubusercontent.com")) {
    console.error("Invalid URL for GitHub Gist");
    return;
  }

  // Check if we've already loaded the data for this state.
  if (hepEVD_GLOBAL_STATE.initialised) {
    return updateExternalData();
  }

  // If not, looks like it is the first time we've loaded the data.
  // Pull down the JSON object from the URL.
  const result = await getDataWithProgress(url);

  // Two possible formats for the data:
  // 1. A single JSON object with all the data.
  // 2. A JSON info object, that points to a list of files, each of which
  //    contains a different event state.

  if (
    !result.hasOwnProperty("numberOfStates") &&
    !result.hasOwnProperty("states")
  ) {
    // This is the first format, so just return the data.
    return result;
  }

  // This is the second format, so we need to load the data the final event state.
  const states = result.states;
  const numberOfStates = result.numberOfStates;

  // Get the last state.
  const lastState = states[numberOfStates - 1].url;
  const lastStateData = await getDataWithProgress(lastState);

  hepEVD_GLOBAL_STATE.state = {
    numberOfStates: numberOfStates,
    currentState: numberOfStates - 1,
    states: states,
    detectorGeometry: result.detectorGeometry,
    config: result.config,
  };

  // Set any citations, if they exist.
  if (result.hasOwnProperty("citation")) {
    hepEVD_GLOBAL_STATE.state.citation = result.citation;
    addCitation(result.citation.text, result.citation.url);
  }

  return {
    hits: lastStateData.hits,
    mcHits: lastStateData.mcHits,
    markers: lastStateData.markers,
    particles: lastStateData.particles,
    images: lastStateData.images || [],
    detectorGeometry: result.detectorGeometry,
    stateInfo: lastStateData.stateInfo || { mcTruth: "" },
    config: result.config || {},
  };
}

// Top-level function to load data from the server or from a GitHub Gist URL.
export async function getData() {
  if (isRunningOnGitHubPages()) {
    // If there is a query string, then load the data from the URL in the query string.
    const urlParams = new URLSearchParams(window.location.search);
    const gistUrl = urlParams.get("data");

    if (gistUrl !== null) {
      return loadExternalData(gistUrl);
    }

    return loadExternalData(
      "https://gist.githubusercontent.com/CrossR/2edd3622d13987d37ef3a4c02286207c/raw/80df485be9f2ca27aa2a8825dbdfdcd794363290/eventDisplayInfo.json",
    );
  } else {
    return loadServerData();
  }
}
