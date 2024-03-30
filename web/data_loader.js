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
export let hepEVD_GLOBAL_STATE;

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

  // Wait for all the data to be loaded.
  [hits, mcHits, markers, particles, detectorGeometry] = await Promise.all([
    hits,
    mcHits,
    markers,
    particles,
    detectorGeometry,
  ]);

  return {
    hits: hits,
    mcHits: mcHits,
    markers: markers,
    particles: particles,
    detectorGeometry: detectorGeometry,
  };
}

/**
 * Updates the external data by fetching new data from the specified URL.
 *
 * @returns {Promise<Object>} An object containing the updated data, including hits, mcHits, markers, particles, and detectorGeometry.
 */
async function updateExternalData() {
  const newDataUrl =
    hepEVD_GLOBAL_STATE.states[hepEVD_GLOBAL_STATE.currentState].url;
  const newStateData = await getDataWithProgress(newDataUrl);

  return {
    hits: newStateData.hits,
    mcHits: newStateData.mcHits,
    markers: newStateData.markers,
    particles: newStateData.particles,
    detectorGeometry: hepEVD_GLOBAL_STATE.detectorGeometry,
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
  if (hepEVD_GLOBAL_STATE !== undefined) {
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

  hepEVD_GLOBAL_STATE = {
    numberOfStates: numberOfStates,
    currentState: numberOfStates - 1,
    states: states,
    detectorGeometry: result.detectorGeometry,
  };

  // Set any citations, if they exist.
  if (result.hasOwnProperty("citation")) {
    hepEVD_GLOBAL_STATE.citation = result.citation;
    addCitation(result.citation.text, result.citation.url);
  }

  return {
    hits: lastStateData.hits,
    mcHits: lastStateData.mcHits,
    markers: lastStateData.markers,
    particles: lastStateData.particles,
    detectorGeometry: result.detectorGeometry,
  };
}

// Top-level function to load data from the server or from a GitHub Gist URL.
export async function getData() {
  if (isRunningOnGitHubPages()) {
    return loadExternalData(
      "https://gist.githubusercontent.com/CrossR/2edd3622d13987d37ef3a4c02286207c/raw/6c5668d3e81280cdad52bccc27d50c0dd576bcc7/eventDisplayInfo.json",
    );
  } else {
    return loadServerData();
  }
}
