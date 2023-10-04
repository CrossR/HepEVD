//
// Event / state management for the web interface.
//

// TODO: Won't work on GitHub pages, since it's not a server.
//       Need to figure out best way to supply multiple states there.

import { getData, isRunningOnGitHubPages } from "./data_loader.js";

/**
 * Updates the UI for the state swapper based on the current state.
 *
 * @param {Function} renderStates - Map of render states.
 * @returns {Promise<void>} - A Promise that resolves when the UI has been updated.
 */
export async function updateStateUI(renderStates) {
  const stateDiv = document.getElementById("stateSwapper");

  // Stop running straight away on GitHub pages.
  if (isRunningOnGitHubPages()) {
    stateDiv.style.display = "none";
    return;
  }

  const stateIdPairs = await getAllStateInfo();

  // Only show the state swapper if there are multiple states.
  if (stateIdPairs.length == 1) {
    stateDiv.style.display = "none";
    return;
  }

  // Since there are multiple states, populate the state swapper.
  const stateList = document.getElementById("state_dropdown");

  // Clear out the old states.
  while (stateList.firstChild) {
    stateList.removeChild(stateList.firstChild);
  }

  // Add the new states.
  stateIdPairs.forEach((idStatePair, index) => {
    const state = idStatePair.state;
    const listElement = document.createElement("li");
    const newButton = document.createElement("li");
    newButton.style.textTransform = "capitalize";
    newButton.innerText = state.name;
    newButton.id = `state_${state.name}_${idStatePair.id}`;
    newButton.addEventListener("click", () =>
      setState(idStatePair.id, renderStates),
    );
    listElement.appendChild(newButton);
    stateList.appendChild(listElement);
  });

  const currentState = await getCurrentStateInfo();
  const currentStateButton = document.getElementById(`state_dropdown_button`);
  currentStateButton.innerText = currentState.name;
}

export async function reloadDataForCurrentState(renderStates) {
  const data = await getData();
  const { hits, mcHits, markers, particles, detectorGeometry } = data;

  const drawTarget = renderStates.get("3D").visible ? "3D" : "2D";

  renderStates.forEach((state) => {
    state.updateData(
      particles,
      hits.filter((hit) => hit.position.dim === state.hitDim),
      mcHits.filter((hit) => hit.position.dim === state.hitDim),
      markers.filter((marker) => marker.position.dim === state.hitDim),
      detectorGeometry,
    );
    state.setupUI(drawTarget, true);
    state.triggerEvent("fullUpdate");
  });
}

/**
 * Retrieves the current state information from the server.
 *
 * @returns {Promise} A Promise that resolves with the JSON state information.
 */
export function getCurrentStateInfo() {
  return fetch("/stateInfo").then((response) => response.json());
}

/**
 * Fetches all state information.
 *
 * @returns {Promise} A Promise that resolves to a map of state IDs to state.
 */
export function getAllStateInfo() {
  return fetch("/allStateInfo").then((response) => response.json());
}

/**
 * Sets the state of the application to the given state ID.
 *
 * @param {number} stateId - The ID of the state to set.
 * @param {Function} renderStates - Map of render states.
 */
export function setState(stateId, renderStates) {
  fetch(`/swap/id/${stateId}`);
  updateStateUI(renderStates);
  reloadDataForCurrentState(renderStates);
}

/**
 * Fetches the next state and updates the UI accordingly.
 *
 * @param {Function} renderStates - Map of render states.
 *
 * @returns {void}
 */
export function nextState(renderStates) {
  fetch("/nextState");
  updateStateUI(renderStates);
  reloadDataForCurrentState(renderStates);
}

/**
 * Fetches the previous state and updates the UI accordingly.
 *
 * @param {Function} renderStates - Map of render states.
 *
 * @returns {void}
 */
export function previousState(renderStates) {
  fetch("/previousState");
  updateStateUI(renderStates);
  reloadDataForCurrentState(renderStates);
}
